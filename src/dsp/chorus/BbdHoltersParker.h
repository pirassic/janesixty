// SPDX-License-Identifier: GPL-3.0-or-later
// Holters and Parker, "A combined model for a bucket brigade device and its input and output
// filters", DAFx-18, Algorithm 1 with the Juno-60 coefficients of their Table 1. The BBD is a
// fixed-length queue clocked at its own variable rate; the input filter (partial fractions,
// first-order complex sections run at the audio rate) is evaluated at each even clock edge to
// give the sampled BBD input, and the output filter (same form) is driven by the steps of the
// BBD's staircase output at each odd edge, so the resampling in both directions uses the
// board's own anti-aliasing filters. Clip at the BBD input is the MN3009 overload (datasheet).
#pragma once

#include <array>
#include <cmath>
#include <complex>
#include <vector>

namespace jane60
{

class BbdHoltersParker
{
public:
    using cplx = std::complex<double>;

    /// Table 1 (rad/s): one real section and two conjugate pairs per filter; we keep one of each pair
    /// and take twice the real part.
    struct Section { cplx r, p; bool pair; };
    static constexpr std::size_t kSections = 3;

    static std::array<Section, kSections> inputSections() noexcept
    {
        return { { { { 251589.0, 0.0 }, { -46580.0, 0.0 }, false },
                   { { -130428.0, -4165.0 }, { -55482.0, 25082.0 }, true },
                   { { 4634.0, -22873.0 }, { -26292.0, -59437.0 }, true } } };
    }
    static std::array<Section, kSections> outputSections() noexcept
    {
        return { { { { 5092.0, 0.0 }, { -176261.0, 0.0 }, false },
                   { { 11256.0, -99566.0 }, { -51468.0, 21437.0 }, true },
                   { { -13802.0, -24606.0 }, { -26276.0, -59699.0 }, true } } };
    }

    /// Shared input filter (one pre-filter feeds both BBDs on the board).
    class InputFilter
    {
    public:
        void prepare (double sampleRate)
        {
            ts_ = 1.0 / sampleRate;
            sec_ = inputSections();
            for (std::size_t m = 0; m < kSections; ++m)
            {
                pbar_[m] = std::exp (sec_[m].p * ts_);
                for (std::size_t i = 0; i <= kLut; ++i)
                    lut_[m][i] = ts_ * sec_[m].r * std::exp (sec_[m].p * (static_cast<double> (i) / kLut) * ts_);
                x_[m] = 0.0;
            }
        }
        /// Audio-rate state update with the new input sample (Algorithm 1, line 21).
        void push (double u) noexcept
        {
            for (std::size_t m = 0; m < kSections; ++m)
                x_[m] = pbar_[m] * x_[m] + u;
        }
        /// Filter output at fractional offset d (0..1] into the current sample interval (line 9).
        double sampleAt (double d) const noexcept
        {
            double y = 0.0;
            for (std::size_t m = 0; m < kSections; ++m)
            {
                const cplx v = interp (lut_[m], d) * x_[m];
                y += sec_[m].pair ? 2.0 * v.real() : v.real();
            }
            return y;
        }
    private:
        friend class BbdHoltersParker;
        static constexpr std::size_t kLut = 256;
        static cplx interp (const std::array<cplx, kLut + 1>& t, double d) noexcept
        {
            const double x = std::max (0.0, d * static_cast<double> (kLut));
            const auto i = static_cast<std::size_t> (x);
            if (i >= kLut) return t[kLut];
            const double f = x - static_cast<double> (i);
            return t[i] + (t[i + 1] - t[i]) * f;
        }
        double ts_ = 1.0 / 48000.0;
        std::array<Section, kSections> sec_ {};
        std::array<cplx, kSections> pbar_ {}, x_ {};
        std::array<std::array<cplx, kLut + 1>, kSections> lut_ {};
    };

    /// One BBD line with its output filter.
    void prepare (double sampleRate, int stages, double maxDelayMs)
    {
        fs_ = sampleRate;
        ts_ = 1.0 / sampleRate;
        stages_ = stages;
        sec_ = outputSections();
        h0_ = 0.0;
        for (std::size_t m = 0; m < kSections; ++m)
        {
            pbar_[m] = std::exp (sec_[m].p * ts_);
            const cplx rp = sec_[m].r / sec_[m].p;
            h0_ -= sec_[m].pair ? 2.0 * rp.real() : rp.real();
            for (std::size_t i = 0; i <= kLut; ++i)
                lut_[m][i] = rp * std::exp (sec_[m].p * (1.0 - static_cast<double> (i) / kLut) * ts_);
            x_[m] = 0.0;
        }
        queue_.assign (static_cast<std::size_t> (std::max (1, stages / 2)) + 2, 0.0);
        write_ = 0;
        yOld_ = 0.0;
        nextEdge_ = 0.0;
        even_ = true;
        (void) maxDelayMs;
    }

    /// One audio sample. in: the shared input filter (already holding samples up to k-1).
    /// delayMs: the BBD delay this sample (the clock LFO). clip: applied to each sampled BBD input.
    template <typename Clip>
    double process (const InputFilter& in, double delayMs, Clip clip) noexcept
    {
        // Edge interval in samples: the clock runs at fBBD = stages / (2 delay), two edges per period.
        const double edgeSamples = std::max (1e-3, fs_ * delayMs * 1e-3 / static_cast<double> (stages_));
        // Edges in (k-1, k]: nextEdge_ counts from the start of the current sample interval.
        while (nextEdge_ <= 1.0)
        {
            const double d = std::max (1e-9, nextEdge_);
            if (even_)
            {
                queue_[write_] = clip (in.sampleAt (d));
                write_ = (write_ + 1) % queue_.size();
            }
            else
            {
                // The sample enqueued stages / 2 - 1 enqueues before the latest one (eq. 1).
                const std::size_t back = static_cast<std::size_t> (stages_ / 2);
                const double y = queue_[(write_ + queue_.size() - back) % queue_.size()];
                const double delta = y - yOld_;
                yOld_ = y;
                for (std::size_t m = 0; m < kSections; ++m)
                    x_[m] += InputFilter::interp (lut_[m], d) * delta;
            }
            even_ = ! even_;
            nextEdge_ += edgeSamples;
        }
        nextEdge_ -= 1.0;
        double y = h0_ * yOld_;
        for (std::size_t m = 0; m < kSections; ++m)
        {
            y += sec_[m].pair ? 2.0 * x_[m].real() : x_[m].real();
            x_[m] *= pbar_[m];
        }
        return y;
    }

private:
    static constexpr std::size_t kLut = InputFilter::kLut;
    double fs_ = 48000.0, ts_ = 1.0 / 48000.0;
    int stages_ = 256;
    std::array<Section, kSections> sec_ {};
    std::array<cplx, kSections> pbar_ {}, x_ {};
    std::array<std::array<cplx, kLut + 1>, kSections> lut_ {};
    double h0_ = 0.0;
    std::vector<double> queue_;
    std::size_t write_ = 0;
    double yOld_ = 0.0;
    double nextEdge_ = 0.0;
    bool even_ = true;
};

} // namespace jane60

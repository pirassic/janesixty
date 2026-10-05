// SPDX-License-Identifier: GPL-3.0-or-later
// 2x decimator: 31-tap half-band FIR (Kaiser 7.0), flat to 15 kHz, -0.6 dB at
// 20 kHz and -74 dB at 34 kHz for a 48 kHz host. Push two samples at the 2x
// rate, read one output.
#pragma once

namespace jane60
{

class HalfBandDecimator
{
public:
    void reset() noexcept
    {
        for (auto& h : hist_) h = 0.0;
        w_ = 0;
    }

    void push (double v) noexcept
    {
        w_ = (w_ + 1) & 31;
        hist_[w_] = v;
    }

    /// Output for the most recent pair of pushes (group delay 15 samples at the 2x rate).
    [[nodiscard]] double output() const noexcept
    {
        static constexpr double c[8] = { 0.313737466, -0.0930905437, 0.0439889791, -0.0215919023,
                                         0.00980408201, -0.00377247227, 0.00106450368, -0.000125861305 };
        double y = 0.5 * at (15);
        for (int k = 0; k < 8; ++k)
            y += c[k] * (at (15 - (2 * k + 1)) + at (15 + (2 * k + 1)));
        return y;
    }

private:
    [[nodiscard]] double at (int back) const noexcept { return hist_[(w_ - back) & 31]; }
    double hist_[32] {};
    int w_ = 0;
};

} // namespace jane60

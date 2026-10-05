// SPDX-License-Identifier: GPL-3.0-or-later
// IR3109 four-pole OTA-C low-pass with BA662 resonance feedback, Juno-60 wiring:
// 68 k / 560 R input attenuators into each OTA pair, 240 pF per stage, inverting
// global feedback with input-side Q compensation (Service Notes p.9).
//
// Model: each stage is dy/dt = wc * tanh(x - y) in units of 2*Vt, discretised with
// the trapezoidal rule (y = s + g * tanh(x - y), s' = 2y - s) and solved per stage
// with Newton steps. The global feedback loop (x1 = in - k * y4) is solved by
// fixed-point iteration from the previous output. Runs at 2x the host rate: the
// input is held for both sub-steps and the output is decimated through a 31-tap
// half-band FIR (Kaiser 7.0: flat to 15 kHz, -0.6 dB at 20 kHz, -74 dB at 34 kHz at
// a 48 kHz host) so the saturation harmonics above the host Nyquist do not fold
// back as inharmonic tones.
#pragma once

#include "dsp/Calibration.h"

#include <cmath>

namespace jane60
{

class Ir3109
{
public:
    void configure (const Calibration::Vcf& c, double sampleRate) noexcept
    {
        cal_ = c;
        sr_ = sampleRate * 2.0;
        attenuation_ = c.shuntResistorOhm / (c.inputResistorOhm + c.shuntResistorOhm);
        reset();
    }

    void reset() noexcept
    {
        for (auto& s : s_) s = 0.0;
        y4_ = 0.0;
        for (auto& h : hist_) h = 0.0;
        w_ = 0;
    }

    /// cutoffHz: cutoff of each one-pole stage. k: feedback gain, 4 = self-oscillation threshold.
    void set (double cutoffHz, double k) noexcept
    {
        if (cutoffHz < 1.0) cutoffHz = 1.0;
        // Above ~0.22 of the oversampled rate (21 kHz at a 48 kHz host) the per-stage Newton
        // solve and the 2x decimation lose accuracy and the tanh harmonics alias into
        // broadband noise. The real filter is fully open there anyway, so the corner holds.
        if (cutoffHz > sr_ * 0.22) cutoffHz = sr_ * 0.22;
        const double T = 1.0 / sr_;
        const double wa = (2.0 / T) * std::tan (3.14159265358979323846 * cutoffHz * T);
        g_ = wa * T / 2.0;
        k_ = k;
    }

    /// Input in volts at the mixer output; output in volts, referred back to the input scale.
    double process (double inVolts) noexcept
    {
        push (tickInternal (inVolts));
        push (tickInternal (inVolts));
        // Half-band FIR centred 15 samples back; even taps are zero except the centre.
        static constexpr double c[8] = { 0.313737466, -0.0930905437, 0.0439889791, -0.0215919023,
                                         0.00980408201, -0.00377247227, 0.00106450368, -0.000125861305 };
        double y = 0.5 * at (15);
        for (int k = 0; k < 8; ++k)
            y += c[k] * (at (15 - (2 * k + 1)) + at (15 + (2 * k + 1)));
        return y;
    }

    [[nodiscard]] double feedbackGain() const noexcept { return k_; }

private:
    // Solve y = s + g tanh(x - y) for y by Newton's method.
    static double solveStage (double x, double s, double g) noexcept
    {
        double y = (s + g * x) / (1.0 + g); // linear estimate
        for (int it = 0; it < 3; ++it)
        {
            const double d = x - y;
            const double th = std::tanh (d);
            const double f = y - s - g * th;
            const double fp = 1.0 + g * (1.0 - th * th);
            y -= f / fp;
        }
        return y;
    }

    double tickInternal (double inVolts) noexcept
    {
        constexpr double vt2 = 2.0 * 0.026;
        const double xin = inVolts * attenuation_ / vt2;
        const double comp = 1.0 + cal_.qCompensation * k_;

        double y4 = y4_;
        double y[4] {};
        for (int it = 0; it < 3; ++it)
        {
            double x = xin * comp - k_ * y4;
            for (int i = 0; i < 4; ++i)
            {
                y[i] = solveStage (x, s_[i], g_);
                x = y[i];
            }
            y4 = y[3];
        }
        for (int i = 0; i < 4; ++i)
            s_[i] = 2.0 * y[i] - s_[i];
        y4_ = y4;
        return y4 * vt2 / attenuation_;
    }

    void push (double v) noexcept
    {
        w_ = (w_ + 1) & 31;
        hist_[w_] = v;
    }
    /// Sample `back` steps before the newest one (0 = newest).
    [[nodiscard]] double at (int back) const noexcept { return hist_[(w_ - back) & 31]; }

    Calibration::Vcf cal_ {};
    double sr_ = 96000.0;
    double hist_[32] {};
    int w_ = 0;
    double attenuation_ = 560.0 / 68560.0;
    double g_ = 0.0;
    double k_ = 0.0;
    double s_[4] {};
    double y4_ = 0.0;
};

} // namespace jane60

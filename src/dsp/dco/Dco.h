// SPDX-License-Identifier: GPL-3.0-or-later
// One Juno-60 DCO: an integrator ramp reset by the 8253 counter, with the
// amplitude-compensation CV from a 7-bit DAC through an anti-log amplifier,
// a comparator for the pulse and a divide-by-two flip-flop for the sub
// (Service Notes p.14 Fig. 2, p.15 Fig. 3). Band-limited with PolyBLEP at 2x the
// host rate and a half-band decimator: the two-point PolyBLEP droops by 3 dB at a
// quarter of its own rate, which at the host rate was audible as a lost top octave.
//
// Polarity: the hardware ramp falls from 0 V to -12 V and resets upward. The
// pulse polarity relative to the saw is provisional (phase 4 checks it against
// the comparator wiring: saw and pulse fundamentals must add when both are on).
#pragma once

#include "dsp/Calibration.h"
#include "dsp/core/Blep.h"
#include "dsp/core/HalfBand.h"

#include <cmath>

namespace jane60
{

class Dco
{
public:
    void configure (const Calibration::Dco& c, double sampleRate) noexcept
    {
        cal_ = c;
        sr_ = sampleRate;
        amplitudeVpp_ = c.sawVpp;
        duty_ = cal_.pulseMinDuty;
    }

    void reset() noexcept
    {
        phase_ = 0.0;
        subHalf_ = false;
        decSaw_.reset();
        decPulse_.reset();
        decSub_.reset();
    }

    void setPitch (long divisor, double clockHz) noexcept
    {
        divisor_ = divisor;
        freqHz_ = clockHz / static_cast<double> (divisor);
        inc_ = freqHz_ / (2.0 * sr_); // phase increment at the 2x rate
        if (inc_ > 0.49) inc_ = 0.49;
    }

    /// Pulse width from the comparator CV in volts (-6.5 V = 50 %, -0.5 V = 97 %).
    void setPwmVolts (double cv) noexcept
    {
        const double span = cal_.pwmCvAt97Pct - cal_.pwmCvAt50Pct;
        double d = 0.50 + 0.47 * (cv - cal_.pwmCvAt50Pct) / span;
        if (d < cal_.pulseMinDuty) d = cal_.pulseMinDuty;
        if (d > 0.99) d = 0.99;
        duty_ = d;
    }

    /// Duty directly, 0.5..0.99 (convenience for the panel mapping).
    void setDuty (double d) noexcept
    {
        if (d < cal_.pulseMinDuty) d = cal_.pulseMinDuty;
        if (d > 0.99) d = 0.99;
        duty_ = d;
    }

    /// Ramp amplitude in volts peak to peak for the current key (DAC staircase).
    void setAmplitudeVpp (double vpp) noexcept { amplitudeVpp_ = vpp; }

    struct Out
    {
        double saw;   ///< volts, centred: +Vpp/2 at reset falling to -Vpp/2
        double pulse; ///< volts, +-Vpp/2
        double sub;   ///< volts, +-Vpp/2, one octave below
    };

    Out tick() noexcept
    {
        for (int i = 0; i < 2; ++i)
        {
            const Out o = tickOnce();
            decSaw_.push (o.saw);
            decPulse_.push (o.pulse);
            decSub_.push (o.sub);
        }
        return { decSaw_.output(), decPulse_.output(), decSub_.output() };
    }

    [[nodiscard]] double frequencyHz() const noexcept { return freqHz_; }
    [[nodiscard]] long divisor() const noexcept { return divisor_; }
    [[nodiscard]] double duty() const noexcept { return duty_; }

private:
    Out tickOnce() noexcept
    {
        const double dt = inc_;
        const double vpp = amplitudeVpp_;
        const double half = vpp * 0.5;

        // The polyBlep residual cancels a step of 2 units (the +-1 textbook saw), so a
        // step of Vpp takes half * residual.
        // Falling ramp with an upward reset step of +Vpp at phase 0.
        double saw = half - vpp * phase_;
        saw += half * polyBlep (phase_, dt);

        // Pulse: high for phase < duty. Rising edge at 0, falling edge at duty.
        double pulse = phase_ < duty_ ? half : -half;
        pulse += half * polyBlep (phase_, dt);
        pulse -= half * polyBlep (wrap (phase_ - duty_), dt);

        // Sub: square at half frequency, phase-locked to the reset.
        const double subPhase = (subHalf_ ? 0.5 : 0.0) + 0.5 * phase_;
        const double dts = 0.5 * dt;
        double sub = subPhase < 0.5 ? half : -half;
        sub += half * polyBlep (subPhase, dts);
        sub -= half * polyBlep (wrap (subPhase - 0.5), dts);

        phase_ += dt;
        if (phase_ >= 1.0)
        {
            phase_ -= 1.0;
            subHalf_ = ! subHalf_;
        }
        return { saw, pulse, sub };
    }

    static double wrap (double t) noexcept { return t < 0.0 ? t + 1.0 : t; }

    Calibration::Dco cal_ {};
    double sr_ = 48000.0;
    double phase_ = 0.0;
    double inc_ = 0.0;
    double freqHz_ = 0.0;
    long divisor_ = 1;
    double duty_ = 0.5;
    bool subHalf_ = false;
    double amplitudeVpp_ = 12.0;
    HalfBandDecimator decSaw_, decPulse_, decSub_;
};

/// The shared noise generator: reverse-biased 2SC945 junction with a mild roll-off.
/// One instance feeds all six voices. Level: 4 Vp-p at the voice VCA output at NOISE 10.
class NoiseSource
{
public:
    void configure (double sampleRate, double vpp) noexcept
    {
        constexpr double pi = 3.14159265358979323846;
        const double fc = 5000.0; // 106-clone note; unconfirmed for the 60 (calibration later)
        a_ = std::exp (-2.0 * pi * fc / sampleRate);
        // Peak-to-peak of filtered gaussian noise ~ 6 sigma; filtered variance ~ (1-a)/(1+a).
        const double filteredSigma = std::sqrt ((1.0 - a_) / (1.0 + a_));
        gain_ = (vpp / 6.0) / filteredSigma;
    }

    double tick() noexcept
    {
        double s = 0.0;
        for (int i = 0; i < 4; ++i)
        {
            state_ ^= state_ << 13;
            state_ ^= state_ >> 17;
            state_ ^= state_ << 5;
            s += static_cast<double> (state_) / 4294967296.0 - 0.5;
        }
        const double white = s * 1.7320508075688772; // unit variance
        lp_ = a_ * lp_ + (1.0 - a_) * white;
        return lp_ * gain_;
    }

private:
    double a_ = 0.0;
    double gain_ = 1.0;
    double lp_ = 0.0;
    unsigned int state_ = 0x9E3779B9u;
};

} // namespace jane60

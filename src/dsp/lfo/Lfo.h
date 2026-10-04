// SPDX-License-Identifier: GPL-3.0-or-later
// The shared LFO: a free-running triangle with a delay circuit that holds the
// depth at zero and then fades it in (research/01 section 7; Owner's Manual p.17).
// AUTO: the delay restarts on the first key of a phrase. MANUAL: output only
// while the LFO TRIG button is held, delay ramp from the press.
#pragma once

#include "dsp/Calibration.h"

#include <cmath>

namespace jane60
{

class Lfo
{
public:
    enum class TrigMode { automatic, manual };

    void configure (const Calibration::Lfo& c, double sampleRate) noexcept
    {
        cal_ = c;
        sr_ = sampleRate;
    }

    void setSliders (double rate, double delay) noexcept
    {
        inc_ = cal_.rateHz.at (rate) / sr_;
        holdSamples_ = cal_.delayHoldSeconds.at (delay) * sr_;
        fadeSamples_ = cal_.delayFadeSeconds.at (delay) * sr_;
        delayActive_ = delay > 0.01;
    }

    void setTrigMode (TrigMode m) noexcept { mode_ = m; }

    /// Call when a key is pressed while no other keys are held (first key of a phrase).
    void phraseStart() noexcept
    {
        if (mode_ == TrigMode::automatic)
            restartDelay();
    }

    /// LFO TRIG button state (MANUAL mode).
    void trigButton (bool down) noexcept
    {
        if (down && ! button_)
            restartDelay();
        button_ = down;
    }

    /// Returns the modulation output, -1..1, already scaled by the delay envelope.
    double tick() noexcept
    {
        // Triangle, free-running.
        phase_ += inc_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        const double tri = phase_ < 0.5 ? 4.0 * phase_ - 1.0 : 3.0 - 4.0 * phase_;

        double depth = 1.0;
        if (mode_ == TrigMode::manual && ! button_)
            depth = 0.0;
        else if (delayActive_)
        {
            if (elapsed_ < holdSamples_)
                depth = 0.0;
            else if (elapsed_ < holdSamples_ + fadeSamples_)
                depth = (elapsed_ - holdSamples_) / fadeSamples_;
            elapsed_ += 1.0;
        }
        return tri * depth;
    }

    [[nodiscard]] double rawTriangle() const noexcept
    {
        return phase_ < 0.5 ? 4.0 * phase_ - 1.0 : 3.0 - 4.0 * phase_;
    }

private:
    void restartDelay() noexcept { elapsed_ = 0.0; }

    Calibration::Lfo cal_ {};
    double sr_ = 48000.0;
    double phase_ = 0.0;
    double inc_ = 0.0;
    double holdSamples_ = 0.0, fadeSamples_ = 1.0;
    double elapsed_ = 1.0e12;
    bool delayActive_ = false;
    TrigMode mode_ = TrigMode::automatic;
    bool button_ = false;
};

} // namespace jane60

// SPDX-License-Identifier: GPL-3.0-or-later
// IR3R01 ADSR as fitted to the Juno-60 measurements (research/01 section 6):
// attack is an RC charge toward an overshoot target, truncated when it reaches
// full level (so the attack time is the time to reach 1.0); decay and release are
// exponential toward the sustain level and zero with a fixed shape constant, so
// decay duration does not depend on sustain. Gate and trigger are tied: a new
// gate restarts the attack from the current level.
#pragma once

#include "dsp/Calibration.h"

#include <cmath>

namespace jane60
{

class Ir3r01Envelope
{
public:
    enum class Stage { idle, attack, decay, sustain, release };

    void configure (const Calibration::Env& c, double sampleRate) noexcept
    {
        cal_ = c;
        sr_ = sampleRate;
    }

    /// Slider positions 0..10 (already quantised by the caller if desired).
    void setSliders (double a, double d, double s, double r) noexcept
    {
        sliders_[0] = a; sliders_[1] = d; sliders_[2] = s; sliders_[3] = r;
        attackT_ = cal_.attackSeconds.at (a) * timeScale_;
        decayT_ = cal_.decaySeconds.at (d) * timeScale_;
        releaseT_ = cal_.releaseSeconds.at (r) * timeScale_;
        sustain_ = cal_.sustainLevel.at (s);
        recompute();
    }

    /// Condition layer: per-voice time tolerance (the IR3R01's timing capacitor and trim).
    void setTimeScale (double scale) noexcept
    {
        timeScale_ = scale;
        setSliders (sliders_[0], sliders_[1], sliders_[2], sliders_[3]);
    }

    void gate (bool on) noexcept
    {
        if (on)
        {
            stage_ = Stage::attack;
        }
        else if (stage_ != Stage::idle)
        {
            stage_ = Stage::release;
        }
        gate_ = on;
    }

    double tick() noexcept
    {
        switch (stage_)
        {
            case Stage::attack:
                level_ += (target_ - level_) * attackCoef_;
                if (level_ >= 1.0)
                {
                    level_ = 1.0;
                    stage_ = Stage::decay;
                }
                break;
            case Stage::decay:
                level_ += (sustain_ - level_) * decayCoef_;
                if (level_ - sustain_ < 1e-4)
                {
                    level_ = sustain_;
                    stage_ = Stage::sustain;
                }
                break;
            case Stage::sustain:
                level_ = sustain_;
                break;
            case Stage::release:
                level_ += (0.0 - level_) * releaseCoef_;
                if (level_ < 1e-5)
                {
                    level_ = 0.0;
                    stage_ = Stage::idle;
                }
                break;
            case Stage::idle:
                break;
        }
        return level_;
    }

    [[nodiscard]] Stage stage() const noexcept { return stage_; }
    [[nodiscard]] double level() const noexcept { return level_; }
    [[nodiscard]] bool active() const noexcept { return stage_ != Stage::idle; }

private:
    void recompute() noexcept
    {
        // Attack: level(t) = target (1 - e^{-t/tau}); reaches 1.0 at t = T,
        // so tau = T / ln(target / (target - 1)).
        target_ = cal_.attackOvershootTarget;
        const double tauA = attackT_ / std::log (target_ / (target_ - 1.0));
        attackCoef_ = 1.0 - std::exp (-1.0 / (tauA * sr_));
        // Decay / release: level = S + (1 - S) e^{-k t / T}: tau = T / k.
        const double tauD = decayT_ / cal_.decayShapeK;
        const double tauR = releaseT_ / cal_.decayShapeK;
        decayCoef_ = 1.0 - std::exp (-1.0 / (tauD * sr_));
        releaseCoef_ = 1.0 - std::exp (-1.0 / (tauR * sr_));
    }

    Calibration::Env cal_ {};
    double sr_ = 48000.0;
    double attackT_ = 0.001, decayT_ = 0.002, releaseT_ = 0.002, sustain_ = 1.0;
    double timeScale_ = 1.0;
    double sliders_[4] { 0.0, 0.0, 10.0, 0.0 };
    double target_ = 1.58;
    double attackCoef_ = 0.0, decayCoef_ = 0.0, releaseCoef_ = 0.0;
    double level_ = 0.0;
    Stage stage_ = Stage::idle;
    bool gate_ = false;
};

} // namespace jane60

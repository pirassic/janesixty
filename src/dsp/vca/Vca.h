// SPDX-License-Identifier: GPL-3.0-or-later
// BA662 voice VCA (gain linear in control current) with ENV or GATE control,
// and the shared patch LEVEL VCA (uPC1252H2) on the chorus board.
#pragma once

#include "dsp/Calibration.h"

#include <cmath>

namespace jane60
{

class Ba662Vca
{
public:
    void configure (const Calibration::Vca& c, double sampleRate) noexcept
    {
        riseCoef_ = 1.0 - std::exp (-1.0 / (c.gateRiseMs * 1e-3 * sampleRate));
        fallCoef_ = 1.0 - std::exp (-1.0 / (c.gateFallMs * 1e-3 * sampleRate));
        gateLevel_ = 0.0;
    }

    /// Gate mode control: a slewed on/off.
    double gateControl (bool gateOn) noexcept
    {
        const double target = gateOn ? 1.0 : 0.0;
        gateLevel_ += (target - gateLevel_) * (gateOn ? riseCoef_ : fallCoef_);
        return gateLevel_;
    }

    [[nodiscard]] bool gateActive() const noexcept { return gateLevel_ > 1e-5; }

private:
    double riseCoef_ = 1.0, fallCoef_ = 1.0;
    double gateLevel_ = 0.0;
};

/// Patch LEVEL slider (-5..+5 on the panel). The measured law is close to a
/// square law over the travel; 0 on the panel is the nominal gain.
inline double levelSliderGain (double sliderMinus5ToPlus5) noexcept
{
    const double x = (sliderMinus5ToPlus5 + 5.0) / 10.0; // 0..1
    const double nominal = 0.25;                         // x = 0.5 -> 0.25 (square law)
    return (x * x) / nominal;
}

} // namespace jane60

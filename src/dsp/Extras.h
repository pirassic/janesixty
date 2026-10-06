// SPDX-License-Identifier: GPL-3.0-or-later
// Opt-in extras the hardware never had (plan 02, phase 5): velocity and MPE. Off by default, and
// the off path is bit-identical to the plain instrument: every scale the extras apply is exactly
// 1.0 when they are off. Stored in the plugin state, never in presets.
#pragma once

#include <cmath>

namespace jane60
{

struct Extras
{
    enum class Destination : int { none = 0, vca = 1, vcfEnv = 2, both = 3 };

    bool velocityOn = false;
    Destination velocityTo = Destination::both;
    double velocityAmount = 1.0;     ///< 0..1: how far a velocity of 0 pulls the destination down
    bool velocitySoft = false;       ///< soft curve (square root) instead of linear

    bool mpeOn = false;              ///< lower zone: master channel 1, member channels 2 to 16
    double mpeBendRangeSemis = 48.0; ///< per-note bend range on member channels (MPE default)
    Destination pressureTo = Destination::vca;
    double pressureAmount = 0.5;     ///< 0..1 (pressure 0 = nothing; pressure 1 = full scale)

    /// Scale 1 - amount (1 - v): 1.0 at full velocity, 1 - amount at zero velocity.
    [[nodiscard]] double velocityScale (double velocity01) const noexcept
    {
        if (! velocityOn) return 1.0;
        const double v = velocitySoft ? std::sqrt (velocity01) : velocity01;
        return 1.0 - velocityAmount * (1.0 - v);
    }
    /// Pressure lifts a destination from (1 - amount) at no pressure to 1.0 at full pressure.
    [[nodiscard]] double pressureScale (double pressure01) const noexcept
    {
        if (! mpeOn || pressureTo == Destination::none) return 1.0;
        return 1.0 - pressureAmount * (1.0 - pressure01);
    }
    static bool toVca (Destination d) noexcept { return d == Destination::vca || d == Destination::both; }
    static bool toVcf (Destination d) noexcept { return d == Destination::vcfEnv || d == Destination::both; }
};

} // namespace jane60

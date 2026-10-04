// SPDX-License-Identifier: GPL-3.0-or-later
// PolyBLEP / PolyBLAMP residuals for band-limited discontinuities.
#pragma once

namespace jane60
{

/// Two-sample polynomial BLEP residual for a unit step at fractional position.
/// t: phase in [0,1), dt: phase increment per sample. Returns the correction to
/// subtract from a naive waveform that steps down by 1 at phase 0.
inline double polyBlep (double t, double dt) noexcept
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0;
    }
    if (t > 1.0 - dt)
    {
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }
    return 0.0;
}

/// PolyBLAMP residual for a unit slope change (used for the finite-slope flyback corner).
inline double polyBlamp (double t, double dt) noexcept
{
    if (t < dt)
    {
        t = t / dt - 1.0;
        return -t * t * t / 3.0;
    }
    if (t > 1.0 - dt)
    {
        t = (t - 1.0) / dt + 1.0;
        return t * t * t / 3.0;
    }
    return 0.0;
}

} // namespace jane60

// SPDX-License-Identifier: GPL-3.0-or-later
// The Juno-60 master oscillator: one LC clock shared by all six voices, pulled in
// cents by the bender, the LFO and the tune control (Service Notes p.14).
// Each voice divides it by an integer (8253 counter), so pitch bend and vibrato
// are perfectly common to all voices and pitch resolution is limited by the
// integer divisor.
#pragma once

#include "dsp/Calibration.h"

#include <cmath>

namespace jane60
{

class MasterClock
{
public:
    void configure (const Calibration::Clock& c, double a4Hz) noexcept
    {
        nominalHz_ = c.masterClockHz;
        // The Service Notes table is built for A4 = 442 Hz. A different tuning reference
        // moves the whole table, which on the hardware is what the rear TUNE knob does.
        referenceRatio_ = a4Hz / c.tuningA4Hz;
        benderMaxCents_ = c.benderMaxCents;
        lfoMaxCents_ = c.lfoMaxCents;
        tuneMaxCents_ = c.tuneMaxCents;
    }

    /// bender: -1..1 (lever), benderDepth: 0..1 (DCO sens slider),
    /// lfo: -1..1 (triangle), lfoDepth: 0..1 (DCO LFO slider, after taper),
    /// tune: -1..1 (rear knob).
    void update (double bender, double benderDepth, double lfo, double lfoDepth, double tune) noexcept
    {
        const double cents = bender * benderDepth * benderMaxCents_
                           + lfo * lfoDepth * lfoMaxCents_
                           + tune * tuneMaxCents_;
        ratio_ = referenceRatio_ * std::exp2 (cents / 1200.0);
    }

    /// Current clock frequency in Hz.
    [[nodiscard]] double hz() const noexcept { return nominalHz_ * ratio_; }
    [[nodiscard]] double nominalHz() const noexcept { return nominalHz_; }

private:
    double nominalHz_ = 1902810.0;
    double referenceRatio_ = 1.0;
    double ratio_ = 1.0;
    double benderMaxCents_ = 700.0, lfoMaxCents_ = 300.0, tuneMaxCents_ = 50.0;
};

/// 8253 divisor for a MIDI note, computed as the firmware's PROM table would be:
/// the divisor that gives the equal-tempered frequency at the nominal clock.
/// The resulting pitch quantisation at the top of the range is part of the model.
struct PitchTable
{
    void configure (double nominalClockHz, double a4Hz) noexcept
    {
        for (int n = 0; n < 128; ++n)
        {
            const double f = a4Hz * std::exp2 ((n - 69) / 12.0);
            divisor[n] = std::max (2L, std::lround (nominalClockHz / f));
        }
    }

    long divisor[128] {};
};

} // namespace jane60

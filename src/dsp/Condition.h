// SPDX-License-Identifier: GPL-3.0-or-later
// The condition layer: properties of the particular unit being emulated, as opposed
// to the panel (the patch) and the calibration file (the circuit). Stored in the
// plugin state, never in presets. Every value here is a documented uncertainty of the
// model (docs/plan/00-overview.md): the exact drive into the IR3109, the BBD noise
// floor, per-voice tolerances, and the unit's VCF trim.
#pragma once

#include "dsp/Calibration.h"

#include <array>

namespace jane60
{

struct Condition
{
    /// VCF trim relative to the Service Notes 248 Hz point, octaves. 0 is the manual's
    /// trim; the calibration file's vcf.trimOffsetOct is the factory demo unit's.
    double vcfTrimOffsetOct = 0.0;

    /// Signal level into the IR3109 OTA pairs relative to the schematic reading (68 k / 560 R
    /// from a 12 Vp-p saw), dB. Only the saturation changes; the output level is compensated.
    /// The mixer legs on p.9 are not fully legible (research/05), so the true level is
    /// uncertain by several dB.
    double vcfDriveDb = 0.0;

    /// Per-voice tolerance scale: 0 = six identical voices, 1 = the tolerances below.
    double voiceSpread = 0.0;

    /// Chorus BBD hiss relative to the calibrated level, dB; chorusNoise false mutes it.
    bool chorusNoise = true;
    double chorusNoiseDb = 0.0;

    static Condition serviceNotes() noexcept { return {}; }
    static Condition demoUnit (const Calibration& cal) noexcept { Condition c; c.vcfTrimOffsetOct = cal.vcf.trimOffsetOct; return c; }

    /// Tolerances at voiceSpread 1, each a documented figure or a stated assumption.
    struct Tolerances
    {
        double sawAmplitude = 1.0 / 12.0;  ///< Service Notes p.23: 12 Vp-p +-1 V across the keyboard
        double cutoffOct = 0.03;           ///< assumed: residual of the per-voice 248 Hz trim, about 2 %
        double resonance = 0.02;           ///< assumed: residual of the per-voice 4 Vp-p resonance trim
        double envTime = 0.08;             ///< KR-106 practical figure (research/03); the Service Notes align the six by eye
        double vcaDb = 0.5;                ///< KR-106 practical figure (research/03); VCA GAIN trimmed per voice
    };

    /// A fixed, reproducible pattern per voice in -1..1 for (saw, cutoff, resonance, envTime, vca).
    static constexpr std::array<std::array<double, 5>, 6> pattern = { {
        { +0.6, -0.3, +0.9, -0.5, +0.2 },
        { -0.8, +0.7, -0.2, +0.4, -0.9 },
        { +0.2, +0.9, -0.7, -0.1, +0.6 },
        { -0.4, -0.6, +0.3, +0.8, -0.3 },
        { +0.9, +0.1, -0.5, -0.9, +0.7 },
        { -0.5, -0.8, +0.6, +0.3, -0.1 } } };
};

/// What one voice deviates from nominal under a Condition (computed by Synth).
struct VoiceDeviation
{
    double sawScale = 1.0;
    double cutoffOct = 0.0;
    double resonanceScale = 1.0;
    double envTimeScale = 1.0;
    double vcaScale = 1.0;
};

} // namespace jane60

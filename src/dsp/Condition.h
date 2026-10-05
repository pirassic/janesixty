// SPDX-License-Identifier: GPL-3.0-or-later
// The condition layer: properties of the particular unit being emulated, as opposed
// to the panel (the patch) and the calibration file (the circuit). Stored in the
// plugin state, never in presets.
#pragma once

#include "dsp/Calibration.h"

namespace jane60
{

struct Condition
{
    /// VCF trim relative to the Service Notes 248 Hz point, octaves. 0 is the manual's
    /// trim; the calibration file's vcf.trimOffsetOct is the factory demo unit's.
    double vcfTrimOffsetOct = 0.0;

    static Condition serviceNotes() noexcept { return {}; }
    static Condition demoUnit (const Calibration& cal) noexcept { return { cal.vcf.trimOffsetOct }; }
};

} // namespace jane60

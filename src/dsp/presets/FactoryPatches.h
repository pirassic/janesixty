// SPDX-License-Identifier: GPL-3.0-or-later
// The 56 factory patches from the Owner's Manual chart, parsed from
// calibration/factory_patches.csv into PanelState values.
#pragma once

#include "dsp/PanelState.h"

#include <string>
#include <string_view>
#include <vector>

namespace jane60
{

struct FactoryPatch
{
    int number = 0;      ///< as printed: 11..78 (bank digit, patch digit)
    std::string name;
    PanelState panel;
};

/// Parse the CSV text. Throws std::runtime_error on a malformed line.
std::vector<FactoryPatch> parseFactoryPatches (std::string_view csvText);

} // namespace jane60

// SPDX-License-Identifier: GPL-3.0-or-later
// Host-agnostic preset format (JSON). A preset stores the patch fields of
// PanelState in the hardware's units, plus a name, bank and tags. Performance
// controls that the hardware does not memorise (arpeggio, hold, key transpose,
// bender depths, volume, tune) are not part of a preset.
#pragma once

#include "dsp/PanelState.h"

#include <string>
#include <string_view>
#include <vector>

namespace jane60
{

struct Preset
{
    static constexpr int kSchema = 1;

    std::string name;
    std::string bank;            ///< "Factory", "User", or any folder-like label
    std::vector<std::string> tags;
    std::string author;
    PanelState panel;            ///< only patch fields are serialised
};

/// Serialise to JSON text (pretty-printed).
std::string presetToJson (const Preset& p);

/// Parse JSON text. Throws std::runtime_error with a message on failure.
Preset presetFromJson (std::string_view jsonText);

/// Copy only the patch fields (what the hardware memorises) from src into dst.
void copyPatchFields (const PanelState& src, PanelState& dst) noexcept;

/// True if the patch fields differ (used for the "edited" indicator).
bool patchFieldsEqual (const PanelState& a, const PanelState& b) noexcept;

} // namespace jane60

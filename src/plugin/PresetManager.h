// SPDX-License-Identifier: GPL-3.0-or-later
// Preset browser model: factory bank (from the chart), user presets as JSON
// files under ~/Library/Audio/Presets/clevergear/Jane-Sixty/, A/B compare and
// the "edited" flag. The audio thread never touches this; the editor and host
// callbacks do.
#pragma once

#include <JuceHeader.h>

#include "dsp/presets/FactoryPatches.h"
#include "dsp/presets/PresetFormat.h"

#include <vector>

namespace jane60
{

class PresetManager final
{
public:
    PresetManager (juce::AudioProcessorValueTreeState& state, const std::vector<FactoryPatch>& factory);

    // --- browsing ---------------------------------------------------------
    struct Entry
    {
        juce::String name;
        juce::String bank;
        juce::File file;   ///< invalid for factory entries
        int factoryIndex = -1;
    };
    const std::vector<Entry>& entries() const noexcept { return entries_; }
    void rescan();

    int currentIndex() const noexcept { return current_; }
    juce::String currentName() const;
    bool isEdited() const;
    void load (int index);
    void loadNext();
    void loadPrevious();

    // --- user presets ------------------------------------------------------
    static juce::File userFolder();
    juce::Result saveAs (const juce::String& name, const juce::String& bank, const juce::StringArray& tags);
    juce::Result importFile (const juce::File& jsonFile);
    juce::Result exportCurrent (const juce::File& jsonFile, const juce::String& name) const;

    // --- A/B ---------------------------------------------------------------
    bool isSlotB() const noexcept { return slotB_; }
    void toggleAB();           ///< swap: current edits go into the active slot, the other slot is recalled
    void copyActiveToOther();  ///< A->B or B->A

    // --- state persistence (called from the processor) ---------------------
    void writeTo (juce::ValueTree& tree) const;
    void readFrom (const juce::ValueTree& tree);

private:
    PanelState snapshot() const;
    void apply (const PanelState& p);

    juce::AudioProcessorValueTreeState& state_;
    const std::vector<FactoryPatch>& factory_;
    std::vector<Entry> entries_;
    int current_ = 0;
    PanelState loaded_;         ///< patch fields as last loaded (for the edited flag)
    PanelState other_;          ///< the inactive A/B slot
    bool slotB_ = false;
};

} // namespace jane60

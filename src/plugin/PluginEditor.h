// SPDX-License-Identifier: GPL-3.0-or-later
// The editor: panel + bender panel + 61-key keyboard, all in reference
// coordinates scaled to the window, with a modern strip below.
#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "ui/PanelComponent.h"
#include "DemoPlayer.h"

namespace jane60
{

class Jane60Editor final : public juce::AudioProcessorEditor,
                           private juce::Timer
{
public:
    explicit Jane60Editor (Jane60Processor&);
    ~Jane60Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress& key) override;

private:
    void timerCallback() override;
    void refreshPresetList();
    void savePreset();

    Jane60Processor& processor_;

    // Everything in reference space lives inside content_, which is scaled as a whole.
    juce::Component content_;
    ui::PanelComponent panel_;
    juce::MidiKeyboardComponent keyboard_;
    DemoPlayer demo_;

    // Modern strip (also in reference space)
    juce::TextButton prev_ { "<" }, next_ { ">" }, ab_ { "A" }, copy_ { "Copy" }, save_ { "Save" }, undo_ { "Undo" };
    juce::ComboBox presetBox_;
    juce::Label edited_, hint_;
    int lastShownIndex_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Jane60Editor)
};

} // namespace jane60

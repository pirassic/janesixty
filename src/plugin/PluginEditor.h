// SPDX-License-Identifier: GPL-3.0-or-later
// Interim editor until the panel UI (phase 3): a preset bar, the host's generic
// parameter list, and an on-screen keyboard that also takes computer-keyboard input.
#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"

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

private:
    void timerCallback() override;
    void refreshPresetList();
    void savePreset();

    Jane60Processor& processor_;
    juce::GenericAudioProcessorEditor generic_;
    juce::MidiKeyboardComponent keyboard_;
    juce::Label hint_;

    juce::TextButton prev_ { "<" }, next_ { ">" }, ab_ { "A/B" }, copy_ { "Copy" }, save_ { "Save" }, undo_ { "Undo" };
    juce::ComboBox presetBox_;
    juce::Label edited_;
    int lastShownIndex_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Jane60Editor)
};

} // namespace jane60

// SPDX-License-Identifier: GPL-3.0-or-later
// Interim editor until the panel UI (phase 3): the host's generic parameter list
// with an on-screen keyboard that also takes computer-keyboard input.
#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"

namespace jane60
{

class Jane60Editor final : public juce::AudioProcessorEditor
{
public:
    explicit Jane60Editor (Jane60Processor&);
    ~Jane60Editor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Jane60Processor& processor_;
    juce::GenericAudioProcessorEditor generic_;
    juce::MidiKeyboardComponent keyboard_;
    juce::Label hint_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Jane60Editor)
};

} // namespace jane60

// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"

namespace jane60
{

/// Phase 0 placeholder editor: shows the product name, the calibration source count,
/// and one slider bound to the single parameter.
class Jane60Editor final : public juce::AudioProcessorEditor
{
public:
    explicit Jane60Editor (Jane60Processor&);
    ~Jane60Editor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Jane60Processor& processor_;
    juce::Slider volume_;
    juce::Label info_;
    juce::AudioProcessorValueTreeState::SliderAttachment volumeAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Jane60Editor)
};

} // namespace jane60

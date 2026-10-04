// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <JuceHeader.h>

#include "dsp/Calibration.h"

namespace jane60
{

/// Phase 0: an empty, host-valid synth that loads its calibration and does nothing else.
class Jane60Processor final : public juce::AudioProcessor
{
public:
    Jane60Processor();
    ~Jane60Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& state() noexcept { return apvts_; }
    const Calibration& calibration() const noexcept { return calibration_; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static Calibration loadEmbeddedCalibration();

    Calibration calibration_;
    juce::AudioProcessorValueTreeState apvts_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Jane60Processor)
};

} // namespace jane60

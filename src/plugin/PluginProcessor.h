// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <JuceHeader.h>

#include "dsp/Calibration.h"
#include "dsp/Synth.h"
#include "dsp/presets/FactoryPatches.h"
#include "PresetManager.h"

#include <atomic>
#include <vector>

namespace jane60
{

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

    // The 56 factory patches are exposed as programs so hosts can recall them by number.
    int getNumPrograms() override { return static_cast<int> (factory_.size()); }
    int getCurrentProgram() override { return currentProgram_; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& state() noexcept { return apvts_; }
    const Calibration& calibration() const noexcept { return calibration_; }
    PresetManager& presets() noexcept { return presets_; }
    juce::UndoManager& undoManager() noexcept { return undo_; }
    juce::MidiKeyboardState& keyboardState() noexcept { return keyboardState_; }

    // Panel controls that are MIDI-like events rather than parameters.
    void setUiBender (double minusOneToOne) noexcept { uiBender_.store (minusOneToOne); uiBenderDirty_.store (true); }
    void setUiLfoTrig (bool down) noexcept { uiLfoTrig_.store (down); uiLfoTrigDirty_.store (true); }

private:
    static Calibration loadEmbeddedCalibration();
    static std::vector<FactoryPatch> loadEmbeddedFactoryPatches();

    Calibration calibration_;
    std::vector<FactoryPatch> factory_;
    juce::UndoManager undo_;
    juce::AudioProcessorValueTreeState apvts_;
    PresetManager presets_;
    Synth synth_;
    juce::MidiKeyboardState keyboardState_;
    std::atomic<double> uiBender_ { 0.0 };
    std::atomic<bool> uiBenderDirty_ { false };
    std::atomic<bool> uiLfoTrig_ { false };
    std::atomic<bool> uiLfoTrigDirty_ { false };
    std::vector<MidiEvent> events_;
    int currentProgram_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Jane60Processor)
};

} // namespace jane60

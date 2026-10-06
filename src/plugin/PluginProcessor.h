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

    // Condition layer (settings panel): stored in the plugin state, never in presets.
    // demoTrim: VCF trim of the factory demo unit (calibration vcf.trimOffsetOct) instead of the
    // Service Notes trim. Default on, so the factory bank sounds like the demo recording.
    void setDemoTrim (bool on) noexcept { demoTrim_.store (on); }
    [[nodiscard]] bool demoTrim() const noexcept { return demoTrim_.load(); }
    void setVoiceSpread (double s) noexcept { voiceSpread_.store (s); }
    [[nodiscard]] double voiceSpread() const noexcept { return voiceSpread_.load(); }
    void setVcfDriveDb (double db) noexcept { vcfDriveDb_.store (db); }
    [[nodiscard]] double vcfDriveDb() const noexcept { return vcfDriveDb_.load(); }
    void setChorusNoiseDb (double db) noexcept { chorusNoiseDb_.store (db); } // below -90 = off
    [[nodiscard]] double chorusNoiseDb() const noexcept { return chorusNoiseDb_.load(); }
    static constexpr double kChorusNoiseOff = -100.0;

    // Extras (velocity, MPE): opt-in, stored in the plugin state, never in presets; off is bit-identical.
    void setVelocity (int destination) noexcept { velocityTo_.store (destination); } // 0 off, 1 VCA, 2 VCF ENV, 3 both
    [[nodiscard]] int velocity() const noexcept { return velocityTo_.load(); }
    void setVelocityAmount (double a) noexcept { velocityAmount_.store (a); }
    [[nodiscard]] double velocityAmount() const noexcept { return velocityAmount_.load(); }
    void setVelocitySoft (bool s) noexcept { velocitySoft_.store (s); }
    [[nodiscard]] bool velocitySoft() const noexcept { return velocitySoft_.load(); }
    void setMpe (bool on) noexcept { mpe_.store (on); }
    [[nodiscard]] bool mpe() const noexcept { return mpe_.load(); }
    void setPressure (int destination) noexcept { pressureTo_.store (destination); } // 0 off, 1 VCA, 2 VCF ENV, 3 both
    [[nodiscard]] int pressure() const noexcept { return pressureTo_.load(); }

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
    std::atomic<bool> demoTrim_ { true };
    std::atomic<double> voiceSpread_ { 1.0 };     // a serviced unit has its tolerances
    std::atomic<double> vcfDriveDb_ { 0.0 };
    std::atomic<double> chorusNoiseDb_ { 0.0 };
    Condition applied_;
    bool conditionApplied_ = false;
    std::atomic<int> velocityTo_ { 0 };
    std::atomic<double> velocityAmount_ { 1.0 };
    std::atomic<bool> velocitySoft_ { false };
    std::atomic<bool> mpe_ { false };
    std::atomic<int> pressureTo_ { 1 };
    Extras appliedExtras_;
    bool extrasApplied_ = false;

    void applyExtras (bool force) noexcept;

    void applyCondition (bool force) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Jane60Processor)
};

} // namespace jane60

// SPDX-License-Identifier: GPL-3.0-or-later
// The whole instrument: six voices -> sum -> HPF -> LEVEL VCA -> (chorus, phase 2) -> volume.
#pragma once

#include "dsp/Calibration.h"
#include "dsp/Condition.h"
#include "dsp/Extras.h"
#include "dsp/PanelState.h"
#include "dsp/chorus/ChorusBoard.h"
#include "dsp/core/MasterClock.h"
#include "dsp/dco/Dco.h"
#include "dsp/hpf/Hpf.h"
#include "dsp/lfo/Lfo.h"
#include "dsp/vca/Vca.h"
#include "dsp/voice/Voice.h"

#include <array>
#include <cstdint>
#include <vector>

namespace jane60
{

struct MidiEvent
{
    enum class Type : std::uint8_t { noteOn, noteOff, pitchBend, allNotesOff, lfoTrig, holdPedal, channelPressure };
    int sampleOffset = 0;
    Type type = Type::noteOn;
    int note = 0;
    double value = 0.0;   // bend -1..1, trig button 1/0, pressure 0..1
    double velocity = 1.0; // note on, 0..1 (the hardware has none: 1.0 unless velocity is on)
    int channel = 1;      // MIDI channel 1..16 (MPE member channels route per note)
};

class Synth
{
public:
    static constexpr int kVoices = 6;

    void prepare (const Calibration& cal, double sampleRate, double a4Hz = 440.0);

    /// Panel state is copied per block; the DSP never holds a pointer into plugin memory.
    void setPanel (const PanelState& p) noexcept { panel_ = p; }

    /// Unit condition (trim and the like). prepare() starts from the calibration file's demo
    /// unit; the plugin applies the user's choice. Cheap, may be called per block.
    void setCondition (const Condition& c) noexcept;
    [[nodiscard]] const Condition& condition() const noexcept { return condition_; }

    /// Opt-in extras (velocity, MPE); off by default and bit-identical when off.
    void setExtras (const Extras& e) noexcept { extras_ = e; }
    [[nodiscard]] const Extras& extras() const noexcept { return extras_; }

    void render (float* left, float* right, int numSamples, const std::vector<MidiEvent>& events);

    [[nodiscard]] int activeVoices() const noexcept;

private:
    void handle (const MidiEvent& e) noexcept;
    void keyDown (int note, double velocity, int channel) noexcept;
    void keyUp (int note) noexcept;
    void voiceOn (int note, double velocity = 1.0, int channel = 1) noexcept;
    void voiceOff (int note) noexcept;
    void releaseUnheld() noexcept;
    void rebuildArpPattern() noexcept;
    void arpTick() noexcept;
    void updateControls() noexcept;
    int transposeSemis() const noexcept;
    static double outputStage (double x) noexcept;
    bool holdActive() const noexcept { return panel_.hold || pedal_; }

    const Calibration* cal_ = nullptr;
    double sr_ = 48000.0;
    PanelState panel_;
    Condition condition_;
    Extras extras_;

    MasterClock clock_;
    PitchTable pitch_;
    Lfo lfo_;
    NoiseSource noise_;
    Hpf hpf_;
    ChorusBoard chorus_;
    OnePoleLp shelfL_, shelfR_;   // provisional output voicing (Calibration::Voicing)
    double shelfGainMinusOne_ = 0.0;
    double sumGain_ = 1.0;        // voice summer and divider to the chorus input (volts at TP8)
    std::array<Voice, kVoices> voices_;

    int nextVoice_ = 0;          // rotary assignment pointer
    double bender_ = 0.0;        // -1..1
    bool pedal_ = false;         // PEDAL HOLD jack

    // Keyboard state: physically held keys and HOLD-latched keys, in press order.
    std::vector<int> physical_;
    std::vector<int> latched_;   // keys sounding (held or latched), press order, max 6 when latched
    bool lastArpOn_ = false;
    bool lastHold_ = false;

    // Arpeggiator
    std::vector<int> arpPattern_;
    std::size_t arpIndex_ = 0;
    double arpPhase_ = 0.0;      // 0..1 within a step
    int arpSounding_ = -1;
    bool arpDirty_ = false;
};

} // namespace jane60

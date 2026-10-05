// SPDX-License-Identifier: GPL-3.0-or-later
// The whole instrument: six voices -> sum -> HPF -> LEVEL VCA -> (chorus, phase 2) -> volume.
#pragma once

#include "dsp/Calibration.h"
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
    enum class Type : std::uint8_t { noteOn, noteOff, pitchBend, allNotesOff, lfoTrig, holdPedal };
    int sampleOffset = 0;
    Type type = Type::noteOn;
    int note = 0;
    double value = 0.0; // bend -1..1, or trig button 1/0
};

class Synth
{
public:
    static constexpr int kVoices = 6;

    void prepare (const Calibration& cal, double sampleRate, double a4Hz = 440.0);

    /// Panel state is copied per block; the DSP never holds a pointer into plugin memory.
    void setPanel (const PanelState& p) noexcept { panel_ = p; }

    void render (float* left, float* right, int numSamples, const std::vector<MidiEvent>& events);

    [[nodiscard]] int activeVoices() const noexcept;

private:
    void handle (const MidiEvent& e) noexcept;
    void keyDown (int note) noexcept;
    void keyUp (int note) noexcept;
    void voiceOn (int note) noexcept;
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

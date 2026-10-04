// SPDX-License-Identifier: GPL-3.0-or-later
// The whole instrument: six voices -> sum -> HPF -> LEVEL VCA -> (chorus, phase 2) -> volume.
#pragma once

#include "dsp/Calibration.h"
#include "dsp/PanelState.h"
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
    enum class Type : std::uint8_t { noteOn, noteOff, pitchBend, allNotesOff, lfoTrig };
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
    void noteOn (int note) noexcept;
    void noteOff (int note) noexcept;
    void updateControls() noexcept;
    int transposeSemis() const noexcept;

    const Calibration* cal_ = nullptr;
    double sr_ = 48000.0;
    PanelState panel_;

    MasterClock clock_;
    PitchTable pitch_;
    Lfo lfo_;
    NoiseSource noise_;
    Hpf hpf_;
    std::array<Voice, kVoices> voices_;

    int nextVoice_ = 0;          // rotary assignment pointer
    int heldKeys_ = 0;           // for the LFO AUTO phrase detection
    double bender_ = 0.0;        // -1..1
};

} // namespace jane60

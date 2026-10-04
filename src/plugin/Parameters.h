// SPDX-License-Identifier: GPL-3.0-or-later
// Parameter IDs and the mapping between the host-facing parameters and PanelState.
// IDs are frozen at the first release; never reorder or rename.
#pragma once

#include <JuceHeader.h>

#include "dsp/PanelState.h"

namespace jane60::params
{

// Sliders 0..10 (LEVEL -5..+5), switches as choices. Names match the panel legends.
inline constexpr const char* lfoRate = "lfoRate";
inline constexpr const char* lfoDelay = "lfoDelay";
inline constexpr const char* lfoTrig = "lfoTrig";
inline constexpr const char* dcoLfo = "dcoLfo";
inline constexpr const char* dcoPwm = "dcoPwm";
inline constexpr const char* pwmMode = "pwmMode";
inline constexpr const char* pulse = "pulse";
inline constexpr const char* saw = "saw";
inline constexpr const char* sub = "sub";
inline constexpr const char* subLevel = "subLevel";
inline constexpr const char* noise = "noise";
inline constexpr const char* hpf = "hpf";
inline constexpr const char* vcfFreq = "vcfFreq";
inline constexpr const char* vcfRes = "vcfRes";
inline constexpr const char* vcfPolarity = "vcfPolarity";
inline constexpr const char* vcfEnv = "vcfEnv";
inline constexpr const char* vcfLfo = "vcfLfo";
inline constexpr const char* vcfKybd = "vcfKybd";
inline constexpr const char* vcaMode = "vcaMode";
inline constexpr const char* vcaLevel = "vcaLevel";
inline constexpr const char* attack = "attack";
inline constexpr const char* decay = "decay";
inline constexpr const char* sustain = "sustain";
inline constexpr const char* release = "release";
inline constexpr const char* chorus = "chorus";
inline constexpr const char* octave = "octave";
inline constexpr const char* benderDco = "benderDco";
inline constexpr const char* benderVcf = "benderVcf";
inline constexpr const char* volume = "volume";
inline constexpr const char* tune = "tune";

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

/// Read every parameter into a PanelState (called once per block on the audio thread).
PanelState readPanel (const juce::AudioProcessorValueTreeState& s);

/// Write a PanelState's patch fields into the parameters (program change, preset load).
void writePanel (juce::AudioProcessorValueTreeState& s, const PanelState& p);

} // namespace jane60::params

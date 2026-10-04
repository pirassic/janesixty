// SPDX-License-Identifier: GPL-3.0-or-later
// Every front-panel control of the Juno-60, in the hardware's own units.
// Sliders are 0..10 (LEVEL is -5..+5), switches are enums. The plugin layer
// fills this from its parameters; the DSP reads only this.
#pragma once

namespace jane60
{

enum class PwmMode { lfo, manual, env };
enum class VcfPolarity { normal, inverted };
enum class VcaMode { env, gate };
enum class LfoTrigMode { automatic, manual };
enum class ChorusSwitch { off, I, II, I_II };
enum class OctaveTranspose { down, normal, up };

struct PanelState
{
    // LFO
    double lfoRate = 5.0;
    double lfoDelay = 0.0;
    LfoTrigMode lfoTrig = LfoTrigMode::automatic;

    // DCO
    double dcoLfo = 0.0;
    double dcoPwm = 0.0;
    PwmMode pwmMode = PwmMode::manual;
    bool pulseOn = false;
    bool sawOn = true;
    bool subOn = false;
    double subLevel = 0.0;
    double noiseLevel = 0.0;

    // HPF
    int hpf = 0; // 0..3

    // VCF
    double vcfFreq = 5.0;
    double vcfRes = 0.0;
    VcfPolarity vcfPolarity = VcfPolarity::normal;
    double vcfEnv = 0.0;
    double vcfLfo = 0.0;
    double vcfKybd = 10.0;

    // VCA
    VcaMode vcaMode = VcaMode::env;
    double vcaLevel = 0.0; // -5..+5

    // ENV
    double attack = 0.0;
    double decay = 5.0;
    double sustain = 10.0;
    double release = 2.0;

    // Chorus
    ChorusSwitch chorus = ChorusSwitch::off;

    // Stored performance setting
    OctaveTranspose octave = OctaveTranspose::normal;

    // Not stored in a patch
    double benderDco = 0.0;   // 0..10
    double benderVcf = 0.0;   // 0..10
    double volume = 8.0;      // 0..10
    double tune = 0.0;        // -1..1 (rear knob)
    double vcfPedalVolts = 3.0;
};

} // namespace jane60

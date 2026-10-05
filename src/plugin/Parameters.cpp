// SPDX-License-Identifier: GPL-3.0-or-later

#include "Parameters.h"

namespace jane60::params
{

namespace
{

using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;

std::unique_ptr<juce::AudioParameterFloat> slider (const char* id, const juce::String& name, float def,
                                                   float lo = 0.0f, float hi = 10.0f)
{
    return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name,
                                                        juce::NormalisableRange<float> (lo, hi, 0.01f), def);
}

std::unique_ptr<juce::AudioParameterChoice> choice (const char* id, const juce::String& name,
                                                    const juce::StringArray& items, int def)
{
    return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def);
}

std::unique_ptr<juce::AudioParameterBool> toggle (const char* id, const juce::String& name, bool def)
{
    return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def);
}

float get (const juce::AudioProcessorValueTreeState& s, const char* id)
{
    if (auto* p = s.getRawParameterValue (id))
        return p->load();
    return 0.0f;
}

int getChoice (const juce::AudioProcessorValueTreeState& s, const char* id)
{
    return static_cast<int> (std::lround (get (s, id)));
}

void set (juce::AudioProcessorValueTreeState& s, const char* id, float plainValue)
{
    if (auto* p = s.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

} // namespace

Layout createLayout()
{
    Layout l;

    l.add (slider (lfoRate, "LFO Rate", 5.0f));
    l.add (slider (lfoDelay, "LFO Delay Time", 0.0f));
    l.add (choice (lfoTrig, "LFO Trig Mode", { "AUTO", "MAN" }, 0));

    l.add (slider (dcoLfo, "DCO LFO", 0.0f));
    l.add (slider (dcoPwm, "DCO PWM", 0.0f));
    l.add (choice (pwmMode, "PWM Mode", { "LFO", "MANUAL", "ENV" }, 1));
    l.add (toggle (pulse, "Pulse", false));
    l.add (toggle (saw, "Sawtooth", true));
    l.add (toggle (sub, "Sub Osc", false));
    l.add (slider (subLevel, "Sub Osc Level", 0.0f));
    l.add (slider (noise, "Noise", 0.0f));

    l.add (choice (hpf, "HPF", { "0", "1", "2", "3" }, 0));

    l.add (slider (vcfFreq, "VCF Freq", 5.0f));
    l.add (slider (vcfRes, "VCF Res", 0.0f));
    l.add (choice (vcfPolarity, "VCF Env Polarity", { "Normal", "Inverted" }, 0));
    l.add (slider (vcfEnv, "VCF Env", 0.0f));
    l.add (slider (vcfLfo, "VCF LFO", 0.0f));
    l.add (slider (vcfKybd, "VCF Kybd", 10.0f));

    l.add (choice (vcaMode, "VCA Mode", { "ENV", "GATE" }, 0));
    l.add (slider (vcaLevel, "VCA Level", 0.0f, -5.0f, 5.0f));

    l.add (slider (attack, "Attack", 0.0f));
    l.add (slider (decay, "Decay", 5.0f));
    l.add (slider (sustain, "Sustain", 10.0f));
    l.add (slider (release, "Release", 2.0f));

    l.add (choice (chorus, "Chorus", { "OFF", "I", "II", "I+II" }, 0));
    l.add (choice (octave, "Octave Transpose", { "DOWN", "NORMAL", "UP" }, 1));

    l.add (slider (benderDco, "Bender DCO", 0.0f));
    l.add (slider (benderVcf, "Bender VCF", 0.0f));
    l.add (slider (volume, "Volume", 10.0f)); // default 10: the master gain sits 7 dB lower since the headroom fix
    l.add (slider (tune, "Tune", 0.0f, -50.0f, 50.0f));

    l.add (toggle (arpOn, "Arpeggio", false));
    l.add (choice (arpMode, "Arpeggio Mode", { "UP", "UP & DOWN", "DOWN" }, 0));
    l.add (choice (arpRange, "Arpeggio Range", { "1", "2", "3" }, 0));
    l.add (slider (arpRate, "Arpeggio Rate", 5.0f));
    l.add (toggle (hold, "Hold", false));
    l.add (choice (keyTranspose, "Key Transpose", { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "C+" }, 0));

    return l;
}

PanelState readPanel (const juce::AudioProcessorValueTreeState& s)
{
    PanelState p;
    p.lfoRate = get (s, lfoRate);
    p.lfoDelay = get (s, lfoDelay);
    p.lfoTrig = getChoice (s, lfoTrig) == 1 ? LfoTrigMode::manual : LfoTrigMode::automatic;

    p.dcoLfo = get (s, dcoLfo);
    p.dcoPwm = get (s, dcoPwm);
    p.pwmMode = static_cast<PwmMode> (getChoice (s, pwmMode));
    p.pulseOn = get (s, pulse) > 0.5f;
    p.sawOn = get (s, saw) > 0.5f;
    p.subOn = get (s, sub) > 0.5f;
    p.subLevel = get (s, subLevel);
    p.noiseLevel = get (s, noise);

    p.hpf = getChoice (s, hpf);

    p.vcfFreq = get (s, vcfFreq);
    p.vcfRes = get (s, vcfRes);
    p.vcfPolarity = static_cast<VcfPolarity> (getChoice (s, vcfPolarity));
    p.vcfEnv = get (s, vcfEnv);
    p.vcfLfo = get (s, vcfLfo);
    p.vcfKybd = get (s, vcfKybd);

    p.vcaMode = static_cast<VcaMode> (getChoice (s, vcaMode));
    p.vcaLevel = get (s, vcaLevel);

    p.attack = get (s, attack);
    p.decay = get (s, decay);
    p.sustain = get (s, sustain);
    p.release = get (s, release);

    p.chorus = static_cast<ChorusSwitch> (getChoice (s, chorus));
    p.octave = static_cast<OctaveTranspose> (getChoice (s, octave));

    p.benderDco = get (s, benderDco);
    p.benderVcf = get (s, benderVcf);
    p.volume = get (s, volume);
    p.tune = get (s, tune) / 50.0;
    p.arpOn = get (s, arpOn) > 0.5f;
    p.arpMode = static_cast<ArpMode> (getChoice (s, arpMode));
    p.arpRange = getChoice (s, arpRange) + 1;
    p.arpRate = get (s, arpRate);
    p.hold = get (s, hold) > 0.5f;
    p.keyTranspose = getChoice (s, keyTranspose);
    return p;
}

void writePanel (juce::AudioProcessorValueTreeState& s, const PanelState& p)
{
    set (s, lfoRate, static_cast<float> (p.lfoRate));
    set (s, lfoDelay, static_cast<float> (p.lfoDelay));
    set (s, lfoTrig, p.lfoTrig == LfoTrigMode::manual ? 1.0f : 0.0f);
    set (s, dcoLfo, static_cast<float> (p.dcoLfo));
    set (s, dcoPwm, static_cast<float> (p.dcoPwm));
    set (s, pwmMode, static_cast<float> (static_cast<int> (p.pwmMode)));
    set (s, pulse, p.pulseOn ? 1.0f : 0.0f);
    set (s, saw, p.sawOn ? 1.0f : 0.0f);
    set (s, sub, p.subOn ? 1.0f : 0.0f);
    set (s, subLevel, static_cast<float> (p.subLevel));
    set (s, noise, static_cast<float> (p.noiseLevel));
    set (s, hpf, static_cast<float> (p.hpf));
    set (s, vcfFreq, static_cast<float> (p.vcfFreq));
    set (s, vcfRes, static_cast<float> (p.vcfRes));
    set (s, vcfPolarity, static_cast<float> (static_cast<int> (p.vcfPolarity)));
    set (s, vcfEnv, static_cast<float> (p.vcfEnv));
    set (s, vcfLfo, static_cast<float> (p.vcfLfo));
    set (s, vcfKybd, static_cast<float> (p.vcfKybd));
    set (s, vcaMode, static_cast<float> (static_cast<int> (p.vcaMode)));
    set (s, vcaLevel, static_cast<float> (p.vcaLevel));
    set (s, attack, static_cast<float> (p.attack));
    set (s, decay, static_cast<float> (p.decay));
    set (s, sustain, static_cast<float> (p.sustain));
    set (s, release, static_cast<float> (p.release));
    set (s, chorus, static_cast<float> (static_cast<int> (p.chorus)));
    set (s, octave, static_cast<float> (static_cast<int> (p.octave)));
}

} // namespace jane60::params

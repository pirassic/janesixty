// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsp/presets/PresetFormat.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <stdexcept>

namespace jane60
{

using json = nlohmann::json;

namespace
{

const char* pwmModeName (PwmMode m) { return m == PwmMode::lfo ? "LFO" : m == PwmMode::env ? "ENV" : "MAN"; }
PwmMode pwmModeFrom (const std::string& s) { return s == "LFO" ? PwmMode::lfo : s == "ENV" ? PwmMode::env : PwmMode::manual; }

const char* chorusName (ChorusSwitch c)
{
    switch (c) { case ChorusSwitch::I: return "I"; case ChorusSwitch::II: return "II"; case ChorusSwitch::I_II: return "I+II"; default: return "OFF"; }
}
ChorusSwitch chorusFrom (const std::string& s)
{
    return s == "I" ? ChorusSwitch::I : s == "II" ? ChorusSwitch::II : s == "I+II" ? ChorusSwitch::I_II : ChorusSwitch::off;
}

const char* octaveName (OctaveTranspose o) { return o == OctaveTranspose::down ? "DOWN" : o == OctaveTranspose::up ? "UP" : "NORMAL"; }
OctaveTranspose octaveFrom (const std::string& s)
{
    return s == "DOWN" ? OctaveTranspose::down : s == "UP" ? OctaveTranspose::up : OctaveTranspose::normal;
}

double req (const json& j, const char* key)
{
    if (! j.contains (key) || ! j[key].is_number())
        throw std::runtime_error (std::string ("preset: missing or non-numeric '") + key + "'");
    return j[key].get<double>();
}

std::string reqStr (const json& j, const char* key)
{
    if (! j.contains (key) || ! j[key].is_string())
        throw std::runtime_error (std::string ("preset: missing or non-string '") + key + "'");
    return j[key].get<std::string>();
}

bool reqBool (const json& j, const char* key)
{
    if (! j.contains (key) || ! j[key].is_boolean())
        throw std::runtime_error (std::string ("preset: missing or non-boolean '") + key + "'");
    return j[key].get<bool>();
}

} // namespace

std::string presetToJson (const Preset& p)
{
    const PanelState& s = p.panel;
    json j;
    j["schema"] = Preset::kSchema;
    j["name"] = p.name;
    j["bank"] = p.bank;
    j["tags"] = p.tags;
    j["author"] = p.author;

    json& pa = j["patch"];
    pa["lfo"] = { { "rate", s.lfoRate }, { "delay", s.lfoDelay }, { "trig", s.lfoTrig == LfoTrigMode::manual ? "MAN" : "AUTO" } };
    pa["dco"] = { { "lfo", s.dcoLfo }, { "pwm", s.dcoPwm }, { "pwmMode", pwmModeName (s.pwmMode) },
                  { "pulse", s.pulseOn }, { "saw", s.sawOn }, { "sub", s.subOn },
                  { "subLevel", s.subLevel }, { "noise", s.noiseLevel } };
    pa["hpf"] = s.hpf;
    pa["vcf"] = { { "freq", s.vcfFreq }, { "res", s.vcfRes }, { "polarity", s.vcfPolarity == VcfPolarity::inverted ? "INV" : "NORMAL" },
                  { "env", s.vcfEnv }, { "lfo", s.vcfLfo }, { "kybd", s.vcfKybd } };
    pa["vca"] = { { "mode", s.vcaMode == VcaMode::gate ? "GATE" : "ENV" }, { "level", s.vcaLevel } };
    pa["env"] = { { "a", s.attack }, { "d", s.decay }, { "s", s.sustain }, { "r", s.release } };
    pa["chorus"] = chorusName (s.chorus);
    pa["octave"] = octaveName (s.octave);
    return j.dump (2);
}

Preset presetFromJson (std::string_view jsonText)
{
    json j;
    try { j = json::parse (jsonText); }
    catch (const json::parse_error& e) { throw std::runtime_error (std::string ("preset: JSON parse error: ") + e.what()); }

    if (! j.contains ("schema") || j["schema"].get<int>() != Preset::kSchema)
        throw std::runtime_error ("preset: unsupported schema");

    Preset p;
    p.name = j.value ("name", "");
    p.bank = j.value ("bank", "");
    p.author = j.value ("author", "");
    if (j.contains ("tags") && j["tags"].is_array())
        for (const auto& t : j["tags"]) if (t.is_string()) p.tags.push_back (t.get<std::string>());

    if (! j.contains ("patch") || ! j["patch"].is_object())
        throw std::runtime_error ("preset: missing 'patch'");
    const json& pa = j["patch"];
    PanelState& s = p.panel;

    const json& lfo = pa.at ("lfo");
    s.lfoRate = req (lfo, "rate"); s.lfoDelay = req (lfo, "delay");
    s.lfoTrig = reqStr (lfo, "trig") == "MAN" ? LfoTrigMode::manual : LfoTrigMode::automatic;

    const json& dco = pa.at ("dco");
    s.dcoLfo = req (dco, "lfo"); s.dcoPwm = req (dco, "pwm"); s.pwmMode = pwmModeFrom (reqStr (dco, "pwmMode"));
    s.pulseOn = reqBool (dco, "pulse"); s.sawOn = reqBool (dco, "saw"); s.subOn = reqBool (dco, "sub");
    s.subLevel = req (dco, "subLevel"); s.noiseLevel = req (dco, "noise");

    s.hpf = static_cast<int> (std::lround (req (pa, "hpf")));
    if (s.hpf < 0 || s.hpf > 3) throw std::runtime_error ("preset: hpf out of range");

    const json& vcf = pa.at ("vcf");
    s.vcfFreq = req (vcf, "freq"); s.vcfRes = req (vcf, "res");
    s.vcfPolarity = reqStr (vcf, "polarity") == "INV" ? VcfPolarity::inverted : VcfPolarity::normal;
    s.vcfEnv = req (vcf, "env"); s.vcfLfo = req (vcf, "lfo"); s.vcfKybd = req (vcf, "kybd");

    const json& vca = pa.at ("vca");
    s.vcaMode = reqStr (vca, "mode") == "GATE" ? VcaMode::gate : VcaMode::env;
    s.vcaLevel = req (vca, "level");

    const json& env = pa.at ("env");
    s.attack = req (env, "a"); s.decay = req (env, "d"); s.sustain = req (env, "s"); s.release = req (env, "r");

    s.chorus = chorusFrom (reqStr (pa, "chorus"));
    s.octave = octaveFrom (reqStr (pa, "octave"));
    return p;
}

void copyPatchFields (const PanelState& src, PanelState& dst) noexcept
{
    dst.lfoRate = src.lfoRate; dst.lfoDelay = src.lfoDelay; dst.lfoTrig = src.lfoTrig;
    dst.dcoLfo = src.dcoLfo; dst.dcoPwm = src.dcoPwm; dst.pwmMode = src.pwmMode;
    dst.pulseOn = src.pulseOn; dst.sawOn = src.sawOn; dst.subOn = src.subOn;
    dst.subLevel = src.subLevel; dst.noiseLevel = src.noiseLevel;
    dst.hpf = src.hpf;
    dst.vcfFreq = src.vcfFreq; dst.vcfRes = src.vcfRes; dst.vcfPolarity = src.vcfPolarity;
    dst.vcfEnv = src.vcfEnv; dst.vcfLfo = src.vcfLfo; dst.vcfKybd = src.vcfKybd;
    dst.vcaMode = src.vcaMode; dst.vcaLevel = src.vcaLevel;
    dst.attack = src.attack; dst.decay = src.decay; dst.sustain = src.sustain; dst.release = src.release;
    dst.chorus = src.chorus; dst.octave = src.octave;
}

bool patchFieldsEqual (const PanelState& a, const PanelState& b) noexcept
{
    auto eq = [] (double x, double y) { return std::abs (x - y) < 1e-6; };
    return eq (a.lfoRate, b.lfoRate) && eq (a.lfoDelay, b.lfoDelay) && a.lfoTrig == b.lfoTrig
        && eq (a.dcoLfo, b.dcoLfo) && eq (a.dcoPwm, b.dcoPwm) && a.pwmMode == b.pwmMode
        && a.pulseOn == b.pulseOn && a.sawOn == b.sawOn && a.subOn == b.subOn
        && eq (a.subLevel, b.subLevel) && eq (a.noiseLevel, b.noiseLevel) && a.hpf == b.hpf
        && eq (a.vcfFreq, b.vcfFreq) && eq (a.vcfRes, b.vcfRes) && a.vcfPolarity == b.vcfPolarity
        && eq (a.vcfEnv, b.vcfEnv) && eq (a.vcfLfo, b.vcfLfo) && eq (a.vcfKybd, b.vcfKybd)
        && a.vcaMode == b.vcaMode && eq (a.vcaLevel, b.vcaLevel)
        && eq (a.attack, b.attack) && eq (a.decay, b.decay) && eq (a.sustain, b.sustain) && eq (a.release, b.release)
        && a.chorus == b.chorus && a.octave == b.octave;
}

} // namespace jane60

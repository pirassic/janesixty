// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsp/presets/FactoryPatches.h"

#include <sstream>
#include <stdexcept>

namespace jane60
{

namespace
{

std::vector<std::string> splitCsv (const std::string& line)
{
    std::vector<std::string> out;
    std::string cur;
    for (char ch : line)
    {
        if (ch == ',') { out.push_back (cur); cur.clear(); }
        else cur += ch;
    }
    out.push_back (cur);
    return out;
}

double num (const std::string& s, const std::string& field, int line)
{
    try { return std::stod (s); }
    catch (...) { throw std::runtime_error ("factory patches line " + std::to_string (line) + ": bad number in " + field); }
}

} // namespace

std::vector<FactoryPatch> parseFactoryPatches (std::string_view csvText)
{
    std::vector<FactoryPatch> result;
    std::istringstream in { std::string (csvText) };
    std::string line;
    int lineNo = 0;
    bool headerSeen = false;
    while (std::getline (in, line))
    {
        ++lineNo;
        if (! line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        if (! headerSeen) { headerSeen = true; continue; }

        const auto f = splitCsv (line);
        if (f.size() != 28)
            throw std::runtime_error ("factory patches line " + std::to_string (lineNo) + ": expected 28 fields, got " + std::to_string (f.size()));

        FactoryPatch p;
        p.number = static_cast<int> (num (f[0], "number", lineNo));
        p.name = f[1];
        PanelState& s = p.panel;
        s.octave = f[2] == "D" ? OctaveTranspose::down : f[2] == "U" ? OctaveTranspose::up : OctaveTranspose::normal;
        s.lfoRate = num (f[3], "lfoRate", lineNo);
        s.lfoDelay = num (f[4], "lfoDelay", lineNo);
        s.lfoTrig = f[5] == "M" ? LfoTrigMode::manual : LfoTrigMode::automatic;
        s.dcoLfo = num (f[6], "dcoLfo", lineNo);
        s.dcoPwm = num (f[7], "pwm", lineNo);
        s.pwmMode = f[8] == "L" ? PwmMode::lfo : f[8] == "E" ? PwmMode::env : PwmMode::manual;
        s.pulseOn = f[9] == "1";
        s.sawOn = f[10] == "1";
        s.subOn = f[11] == "1";
        s.subLevel = num (f[12], "subLevel", lineNo);
        s.noiseLevel = num (f[13], "noise", lineNo);
        s.hpf = static_cast<int> (num (f[14], "hpf", lineNo));
        s.vcfFreq = num (f[15], "vcfFreq", lineNo);
        s.vcfRes = num (f[16], "vcfRes", lineNo);
        s.vcfPolarity = f[17] == "I" ? VcfPolarity::inverted : VcfPolarity::normal;
        s.vcfEnv = num (f[18], "vcfEnv", lineNo);
        s.vcfLfo = num (f[19], "vcfLfo", lineNo);
        s.vcfKybd = num (f[20], "vcfKybd", lineNo);
        s.vcaMode = f[21] == "G" ? VcaMode::gate : VcaMode::env;
        s.vcaLevel = num (f[22], "vcaLevel", lineNo);
        s.attack = num (f[23], "attack", lineNo);
        s.decay = num (f[24], "decay", lineNo);
        s.sustain = num (f[25], "sustain", lineNo);
        s.release = num (f[26], "release", lineNo);
        const int ch = static_cast<int> (num (f[27], "chorus", lineNo));
        s.chorus = ch == 1 ? ChorusSwitch::I : ch == 2 ? ChorusSwitch::II : ch == 3 ? ChorusSwitch::I_II : ChorusSwitch::off;
        result.push_back (p);
    }
    return result;
}

} // namespace jane60

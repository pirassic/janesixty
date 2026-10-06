// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsp/Calibration.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace jane60
{

using json = nlohmann::json;

double SliderTable::at (double slider) const noexcept
{
    if (slider <= 0.0) return values.front();
    if (slider >= 10.0) return values.back();
    const auto lo = static_cast<std::size_t> (std::floor (slider));
    const auto frac = slider - static_cast<double> (lo);
    return values[lo] + (values[lo + 1] - values[lo]) * frac;
}

namespace
{

// Each entry in the JSON is either a plain value or an object {"value": ..., "source": "..."}.
// This helper unwraps both and records the source tag.
class Reader
{
public:
    Reader (const json& root, std::map<std::string, std::string>& sources)
        : root_ (root), sources_ (sources) {}

    const json& node (const std::string& dottedKey) const
    {
        const json* n = &root_;
        std::stringstream ss (dottedKey);
        std::string part;
        while (std::getline (ss, part, '.'))
        {
            if (! n->is_object() || ! n->contains (part))
                throw std::runtime_error ("calibration: missing key '" + dottedKey + "'");
            n = &(*n)[part];
        }
        return *n;
    }

    const json& value (const std::string& key) const
    {
        const json& n = node (key);
        if (n.is_object() && n.contains ("value"))
        {
            if (n.contains ("source"))
                sources_[key] = n["source"].get<std::string>();
            return n["value"];
        }
        return n;
    }

    double number (const std::string& key) const
    {
        const json& v = value (key);
        if (! v.is_number())
            throw std::runtime_error ("calibration: '" + key + "' is not a number");
        return v.get<double>();
    }

    int integer (const std::string& key) const
    {
        return static_cast<int> (std::lround (number (key)));
    }

    SliderTable table (const std::string& key) const
    {
        const json& v = value (key);
        if (! v.is_array() || v.size() != 11)
            throw std::runtime_error ("calibration: '" + key + "' must be an array of 11 numbers");
        SliderTable t;
        for (std::size_t i = 0; i < 11; ++i)
            t.values[i] = v[i].get<double>();
        return t;
    }

    bool has (const std::string& key) const
    {
        try { (void) node (key); return true; }
        catch (const std::runtime_error&) { return false; }
    }

private:
    const json& root_;
    std::map<std::string, std::string>& sources_;
};

} // namespace

Calibration Calibration::fromJson (std::string_view jsonText)
{
    json root;
    try
    {
        root = json::parse (jsonText);
    }
    catch (const json::parse_error& e)
    {
        throw std::runtime_error (std::string ("calibration: JSON parse error: ") + e.what());
    }

    Calibration c;
    Reader r (root, c.sources);

    c.schema = r.integer ("schema");
    if (c.schema != 1)
        throw std::runtime_error ("calibration: unsupported schema " + std::to_string (c.schema));

    c.clock.masterClockHz  = r.number ("clock.masterClockHz");
    c.clock.tuningA4Hz     = r.number ("clock.tuningA4Hz");
    c.clock.benderMaxCents = r.number ("clock.benderMaxCents");
    c.clock.lfoMaxCents    = r.number ("clock.lfoMaxCents");
    c.clock.tuneMaxCents   = r.number ("clock.tuneMaxCents");

    c.dco.sawVpp                = r.number ("dco.sawVpp");
    c.dco.pwmCvAt50Pct          = r.number ("dco.pwmCvAt50Pct");
    c.dco.pwmCvAt97Pct          = r.number ("dco.pwmCvAt97Pct");
    c.dco.pulseMinDuty          = r.number ("dco.pulseMinDuty");
    c.dco.pulseMaxDuty          = r.number ("dco.pulseMaxDuty");
    c.dco.kcvDacBits            = r.number ("dco.kcvDacBits");
    c.dco.kcvVoltsPerOctave     = r.number ("dco.kcvVoltsPerOctave");
    c.dco.resetCapNf            = r.number ("dco.resetCapNf");
    c.dco.resetPulseCapPf       = r.number ("dco.resetPulseCapPf");
    c.dco.resetPulseResistorOhm = r.number ("dco.resetPulseResistorOhm");
    c.dco.noiseVppAtMax         = r.number ("dco.noiseVppAtMax");

    c.vcf.inputResistorOhm        = r.number ("vcf.inputResistorOhm");
    c.vcf.shuntResistorOhm        = r.number ("vcf.shuntResistorOhm");
    c.vcf.stageCapPf              = r.number ("vcf.stageCapPf");
    c.vcf.qCompensation           = r.number ("vcf.qCompensation");
    c.vcf.stage1DriveDb           = r.number ("vcf.stage1DriveDb");
    c.vcf.anchorSliderPos         = r.number ("vcf.anchorSliderPos");
    c.vcf.anchorHz                = r.number ("vcf.anchorHz");
    c.vcf.trimOffsetOct           = r.number ("vcf.trimOffsetOct");
    c.vcf.selfOscVpp              = r.number ("vcf.selfOscVpp");
    c.vcf.keyFollowPivotNote      = r.integer ("vcf.keyFollowPivotNote");
    c.vcf.keyFollowOctPerOct      = r.number ("vcf.keyFollowOctPerOct");
    c.vcf.lfoFullDepthOct         = r.number ("vcf.lfoFullDepthOct");
    c.vcf.envFullPeakHz           = r.number ("vcf.envFullPeakHz");
    c.vcf.pedalDefaultVolts       = r.number ("vcf.pedalDefaultVolts");
    c.vcf.passbandLossAtMaxResDb  = r.number ("vcf.passbandLossAtMaxResDb");

    {
        const json& v = r.value ("hpf.cornerHz");
        if (! v.is_array() || v.size() != 4)
            throw std::runtime_error ("calibration: 'hpf.cornerHz' must have 4 entries");
        for (std::size_t i = 0; i < 4; ++i)
            c.hpf.cornerHz[i] = v[i].get<double>();
    }

    c.vca.voiceOutVpp = r.number ("vca.voiceOutVpp");
    c.vca.sumGainPerVoice = r.number ("vca.sumGainPerVoice");
    c.vca.sumToChorusInput = r.number ("vca.sumToChorusInput");
    c.vca.gateRiseMs  = r.number ("vca.gateRiseMs");
    c.vca.gateFallMs  = r.number ("vca.gateFallMs");

    c.env.attackSeconds         = r.table ("env.attackSeconds");
    c.env.decaySeconds          = r.table ("env.decaySeconds");
    c.env.releaseSeconds        = r.table ("env.releaseSeconds");
    c.env.sustainLevel          = r.table ("env.sustainLevel");
    c.env.attackOvershootTarget = r.number ("env.attackOvershootTarget");
    c.env.decayShapeK           = r.number ("env.decayShapeK");
    c.env.timingCapNf           = r.number ("env.timingCapNf");

    c.lfo.rateHz           = r.table ("lfo.rateHz");
    c.lfo.delayHoldSeconds = r.table ("lfo.delayHoldSeconds");
    c.lfo.delayFadeSeconds = r.table ("lfo.delayFadeSeconds");
    c.lfo.dcoLfoGainVpp    = r.number ("lfo.dcoLfoGainVpp");

    {
        const json& modes = r.value ("chorus.modes");
        if (! modes.is_array() || modes.size() != 3)
            throw std::runtime_error ("calibration: 'chorus.modes' must have 3 entries");
        for (const auto& m : modes)
        {
            ChorusMode cm;
            cm.name       = m.at ("name").get<std::string>();
            cm.lfoRateHz  = m.at ("lfoRateHz").get<double>();
            cm.delayMinMs = m.at ("delayMinMs").get<double>();
            cm.delayMaxMs = m.at ("delayMaxMs").get<double>();
            cm.stereo     = m.at ("stereo").get<bool>();
            c.chorus.modes.push_back (cm);
        }
        if (modes[0].is_object() && root["chorus"]["modes"].is_array())
        {
            // Source tag for the mode table as a whole, if present alongside.
            if (root["chorus"].contains ("modesSource"))
                c.sources["chorus.modes"] = root["chorus"]["modesSource"].get<std::string>();
        }
    }
    c.chorus.bbdStages     = r.integer ("chorus.bbdStages");
    c.chorus.bbdPathGainDb = r.number ("chorus.bbdPathGainDb");
    c.chorus.dryGain       = r.number ("chorus.dryGain");
    c.chorus.wetGain       = r.number ("chorus.wetGain");
    c.chorus.bbdClipVpp    = r.number ("chorus.bbdClipVpp");
    c.chorus.bbdClipRoomV  = r.number ("chorus.bbdClipRoomV");
    c.chorus.noiseDbRe4Vpp = r.number ("chorus.noiseDbRe4Vpp");
    c.chorus.bbdSincBandwidth = r.number ("chorus.bbdSincBandwidth") > 0.5;
    c.chorus.preRealHz  = r.number ("chorus.preRealHz");
    c.chorus.preAHz     = r.number ("chorus.preAHz");
    c.chorus.preAQ      = r.number ("chorus.preAQ");
    c.chorus.preBHz     = r.number ("chorus.preBHz");
    c.chorus.preBQ      = r.number ("chorus.preBQ");
    c.chorus.postAHz    = r.number ("chorus.postAHz");
    c.chorus.postAQ     = r.number ("chorus.postAQ");
    c.chorus.postBHz    = r.number ("chorus.postBHz");
    c.chorus.postBQ     = r.number ("chorus.postBQ");
    c.chorus.postRealHz = r.number ("chorus.postRealHz");

    if (root.contains ("voicing"))
    {
        c.voicing.lowShelfDb = r.number ("voicing.lowShelfDb");
        c.voicing.lowShelfHz = r.number ("voicing.lowShelfHz");
    }

    c.firmware.sliderBits  = r.integer ("firmware.sliderBits");
    c.firmware.panelLoopMs = r.number ("firmware.panelLoopMs");
    c.firmware.voices      = r.integer ("firmware.voices");

    return c;
}

Calibration Calibration::fromFile (const std::string& path)
{
    std::ifstream in (path);
    if (! in)
        throw std::runtime_error ("calibration: cannot open '" + path + "'");
    std::stringstream ss;
    ss << in.rdbuf();
    return fromJson (ss.str());
}

std::vector<std::string> Calibration::assumedKeys() const
{
    std::vector<std::string> keys;
    for (const auto& [k, v] : sources)
        if (v.rfind ("assumed", 0) == 0)
            keys.push_back (k);
    return keys;
}

} // namespace jane60

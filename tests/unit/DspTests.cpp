// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsp/Calibration.h"
#include "dsp/Synth.h"
#include "dsp/core/MasterClock.h"
#include "dsp/dco/Dco.h"
#include "dsp/env/Ir3r01Envelope.h"
#include "dsp/lfo/Lfo.h"
#include "dsp/vcf/Ir3109.h"
#include "dsp/voice/Voice.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <complex>
#include <vector>

using namespace jane60;
using Catch::Matchers::WithinRel;
using Catch::Matchers::WithinAbs;

namespace
{
const Calibration& cal()
{
    static Calibration c = Calibration::fromFile (JANE60_CALIBRATION_FILE);
    return c;
}

// Frequency of the strongest bin via a plain DFT around a search range (test helper, slow but simple).
double dominantFrequency (const std::vector<double>& x, double sr, double fLo, double fHi, double stepHz)
{
    double bestF = fLo, bestMag = -1.0;
    for (double f = fLo; f <= fHi; f += stepHz)
    {
        std::complex<double> acc (0.0, 0.0);
        const double w = 2.0 * 3.14159265358979323846 * f / sr;
        for (std::size_t n = 0; n < x.size(); ++n)
            acc += x[n] * std::polar (1.0, -w * static_cast<double> (n));
        const double mag = std::abs (acc);
        if (mag > bestMag) { bestMag = mag; bestF = f; }
    }
    return bestF;
}

// Count zero crossings (rising) to estimate frequency.
double zeroCrossingFrequency (const std::vector<double>& x, double sr, std::size_t skip)
{
    std::size_t first = 0, last = 0, count = 0;
    for (std::size_t n = skip + 1; n < x.size(); ++n)
    {
        if (x[n - 1] < 0.0 && x[n] >= 0.0)
        {
            if (count == 0) first = n;
            last = n;
            ++count;
        }
    }
    if (count < 2) return 0.0;
    return sr * static_cast<double> (count - 1) / static_cast<double> (last - first);
}
} // namespace

TEST_CASE ("pitch table: A4 divisor is 4305 and C4 is one fifth-plus below")
{
    PitchTable t;
    t.configure (cal().clock.masterClockHz, cal().clock.tuningA4Hz);
    CHECK (t.divisor[69] == 4305);
    // Service Notes DCB table: C4 = 262.8 Hz at A4 = 442.
    CHECK_THAT (cal().clock.masterClockHz / static_cast<double> (t.divisor[60]), WithinRel (262.8, 0.002));
}

TEST_CASE ("master clock: bender at full depth moves the clock by +-700 cents")
{
    MasterClock c;
    c.configure (cal().clock, 442.0);
    c.update (1.0, 1.0, 0.0, 0.0, 0.0);
    CHECK_THAT (c.hz() / c.nominalHz(), WithinRel (std::exp2 (700.0 / 1200.0), 1e-9));
    c.update (-1.0, 1.0, 0.0, 0.0, 0.0);
    CHECK_THAT (c.hz() / c.nominalHz(), WithinRel (std::exp2 (-700.0 / 1200.0), 1e-9));
    c.update (0.0, 0.0, 1.0, 1.0, 1.0);
    CHECK_THAT (c.hz() / c.nominalHz(), WithinRel (std::exp2 (350.0 / 1200.0), 1e-9));
}

TEST_CASE ("DCO: sawtooth period matches the divisor and amplitude is 12 Vp-p")
{
    const double sr = 96000.0;
    Dco d;
    d.configure (cal().dco, sr);
    d.reset();
    PitchTable t;
    t.configure (cal().clock.masterClockHz, 442.0);
    d.setPitch (t.divisor[69], cal().clock.masterClockHz);
    CHECK_THAT (d.frequencyHz(), WithinRel (442.0, 0.001));

    // A band-limited ramp overshoots at its edge (Gibbs), so the amplitude is checked by
    // RMS: a 12 Vp-p ramp has an RMS of 12 / sqrt (12).
    std::vector<double> saw;
    double sq = 0.0;
    for (int i = 0; i < 96000; ++i)
    {
        const auto o = d.tick();
        saw.push_back (o.saw);
        if (i >= 1000) sq += o.saw * o.saw;
    }
    CHECK_THAT (std::sqrt (sq / 95000.0), WithinRel (12.0 / std::sqrt (12.0), 0.02));
    CHECK_THAT (zeroCrossingFrequency (saw, sr, 1000), WithinRel (442.0, 0.01));
}

TEST_CASE ("DCO: sub oscillator is one octave below and the pulse duty follows the comparator CV")
{
    const double sr = 96000.0;
    Dco d;
    d.configure (cal().dco, sr);
    d.reset();
    d.setPitch (4305, cal().clock.masterClockHz);
    d.setPwmVolts (-6.5);
    CHECK_THAT (d.duty(), WithinAbs (0.5, 1e-9));
    d.setPwmVolts (-0.5);
    CHECK_THAT (d.duty(), WithinAbs (0.97, 1e-9));
    d.setDuty (0.5);
    std::vector<double> sub;
    for (int i = 0; i < 96000; ++i)
        sub.push_back (d.tick().sub);
    CHECK_THAT (zeroCrossingFrequency (sub, sr, 1000), WithinRel (221.0, 0.01));
}

TEST_CASE ("IR3109: self-oscillates at the set cutoff with k = 4")
{
    const double sr = 48000.0;
    Ir3109 f;
    f.configure (cal().vcf, sr);
    const VcfMapping map;
    f.set (248.0 * map.selfOscShift, map.kMax);
    std::vector<double> y;
    double x = 0.05; // a small kick
    for (int i = 0; i < 3 * 48000; ++i)
    {
        y.push_back (f.process (x));
        x = 0.0;
    }
    std::vector<double> tail (y.begin() + 2 * 48000, y.end());
    const double fosc = dominantFrequency (tail, sr, 200.0, 300.0, 0.5);
    CHECK_THAT (fosc, WithinRel (248.0, 0.03));
    // Amplitude set by the OTA saturation at the resonance trim target (4 Vp-p, adj. 8-1).
    double mx = 0.0;
    for (double v : tail) mx = std::max (mx, std::abs (v));
    CHECK_THAT (2.0 * mx, WithinRel (cal().vcf.selfOscVpp, 0.1));
}

TEST_CASE ("IR3109: low-pass response, -24 dB/oct slope well above cutoff")
{
    const double sr = 48000.0;
    auto gainAt = [&] (double fIn, double fc)
    {
        Ir3109 f;
        f.configure (cal().vcf, sr);
        f.set (fc, 0.0);
        const double amp = 0.1; // small signal, linear region
        double sumIn = 0.0, sumOut = 0.0;
        for (int i = 0; i < 48000; ++i)
        {
            const double in = amp * std::sin (2.0 * 3.14159265358979323846 * fIn * i / sr);
            const double out = f.process (in);
            if (i >= 24000) { sumIn += in * in; sumOut += out * out; }
        }
        return 10.0 * std::log10 (sumOut / sumIn);
    };
    const double g1 = gainAt (4000.0, 1000.0);
    const double g2 = gainAt (8000.0, 1000.0);
    CHECK (g1 < -20.0);
    CHECK_THAT (g1 - g2, WithinAbs (24.0, 4.0));
    CHECK (gainAt (100.0, 1000.0) > -1.0);
}

TEST_CASE ("IR3R01: attack at slider 10 reaches full level in 3 s, decay independent of sustain")
{
    const double sr = 48000.0;
    Ir3r01Envelope e;
    e.configure (cal().env, sr);
    e.setSliders (10.0, 0.0, 10.0, 0.0);
    e.gate (true);
    int n = 0;
    while (e.stage() == Ir3r01Envelope::Stage::attack && n < 10 * 48000) { e.tick(); ++n; }
    CHECK_THAT (n / sr, WithinRel (3.0, 0.02));

    // Decay from 1.0 to within 1 % of sustain, slider 5: should be about the table time
    // regardless of sustain level.
    auto decayTime = [&] (double s)
    {
        Ir3r01Envelope d;
        d.configure (cal().env, sr);
        d.setSliders (0.0, 5.0, s, 0.0);
        d.gate (true);
        int k = 0;
        while (d.stage() != Ir3r01Envelope::Stage::sustain && k < 60 * 48000) { d.tick(); ++k; }
        return k / sr;
    };
    const double t0 = decayTime (0.0);
    const double t5 = decayTime (5.0);
    CHECK (t0 > 0.5);
    CHECK_THAT (t5, WithinRel (t0, 0.25));
}

TEST_CASE ("LFO: rate at slider 10 is 22 Hz and delay holds then fades")
{
    const double sr = 48000.0;
    Lfo l;
    l.configure (cal().lfo, sr);
    l.setSliders (10.0, 0.0);
    std::vector<double> y;
    for (int i = 0; i < 48000; ++i) y.push_back (l.tick());
    CHECK_THAT (zeroCrossingFrequency (y, sr, 100), WithinRel (22.0, 0.02));

    Lfo d;
    d.configure (cal().lfo, sr);
    d.setSliders (5.0, 10.0);
    d.phraseStart();
    double early = 0.0, late = 0.0;
    for (int i = 0; i < 4 * 48000; ++i)
    {
        const double v = std::abs (d.tick());
        if (i < 48000) early = std::max (early, v);
        if (i > 3 * 48000) late = std::max (late, v);
    }
    CHECK (early < 1e-9); // 2 s hold at slider 10
    CHECK (late > 0.9);
}

TEST_CASE ("Synth: a note produces sound, rotary allocation, six voices, silence after release")
{
    Synth s;
    PanelState p;
    p.sawOn = true;
    p.vcfFreq = 10.0;
    p.attack = 0.0; p.decay = 0.0; p.sustain = 10.0; p.release = 0.0;
    s.setPanel (p);
    s.prepare (cal(), 48000.0, 440.0);

    std::vector<float> l (4800), r (4800);
    std::vector<MidiEvent> ev;
    for (int n = 0; n < 7; ++n)
        ev.push_back ({ 0, MidiEvent::Type::noteOn, 48 + n, 0.0 });
    s.render (l.data(), r.data(), 4800, ev);
    CHECK (s.activeVoices() == 6);
    double peak = 0.0;
    for (float v : l) peak = std::max (peak, std::abs (static_cast<double> (v)));
    CHECK (peak > 0.01);
    CHECK (peak < 1.0);

    ev.clear();
    for (int n = 0; n < 7; ++n)
        ev.push_back ({ 0, MidiEvent::Type::noteOff, 48 + n, 0.0 });
    s.render (l.data(), r.data(), 4800, ev);
    std::vector<MidiEvent> none;
    for (int b = 0; b < 20; ++b) s.render (l.data(), r.data(), 4800, none);
    CHECK (s.activeVoices() == 0);
    double tailPeak = 0.0;
    for (float v : l) tailPeak = std::max (tailPeak, std::abs (static_cast<double> (v)));
    CHECK (tailPeak < 1e-4);
}

#include "dsp/presets/FactoryPatches.h"
#include <fstream>
#include <sstream>

TEST_CASE ("factory patches: all 56 parse with the chart's values")
{
    std::ifstream in (JANE60_FACTORY_PATCHES_FILE);
    REQUIRE (in.good());
    std::stringstream ss;
    ss << in.rdbuf();
    const auto patches = parseFactoryPatches (ss.str());
    REQUIRE (patches.size() == 56);
    CHECK (patches[0].number == 11);
    CHECK (patches[0].name == "Strings 1");
    CHECK (patches[0].panel.sawOn);
    CHECK_FALSE (patches[0].panel.pulseOn);
    CHECK_THAT (patches[0].panel.vcfFreq, WithinAbs (7.0, 1e-9));
    CHECK (patches[0].panel.chorus == ChorusSwitch::I);
    CHECK (patches[5].number == 16);
    CHECK (patches[5].panel.octave == OctaveTranspose::up);
    CHECK (patches[5].panel.vcaMode == VcaMode::gate);
    CHECK (patches[55].number == 78);
    CHECK (patches[55].panel.pwmMode == PwmMode::env);
    CHECK (patches[55].panel.lfoTrig == LfoTrigMode::manual);
    CHECK_THAT (patches[55].panel.vcfRes, WithinAbs (10.0, 1e-9));
    // Patch 62 has inverted VCF polarity and LEVEL +2.
    CHECK (patches[41].number == 62);
    CHECK (patches[41].panel.vcfPolarity == VcfPolarity::inverted);
    CHECK_THAT (patches[41].panel.vcaLevel, WithinAbs (2.0, 1e-9));
}

TEST_CASE ("every factory patch renders without overload or silence where sound is expected")
{
    std::ifstream in (JANE60_FACTORY_PATCHES_FILE);
    std::stringstream ss;
    ss << in.rdbuf();
    const auto patches = parseFactoryPatches (ss.str());
    for (const auto& fp : patches)
    {
        Synth s;
        s.setPanel (fp.panel);
        s.prepare (cal(), 48000.0, 440.0);
        std::vector<float> l (48000), r (48000);
        std::vector<MidiEvent> ev { { 0, MidiEvent::Type::noteOn, 60, 0.0 } };
        s.render (l.data(), r.data(), 48000, ev);
        double peak = 0.0;
        for (float v : l)
        {
            REQUIRE (std::isfinite (v));
            peak = std::max (peak, std::abs (static_cast<double> (v)));
        }
        INFO ("patch " << fp.number << " " << fp.name);
        CHECK (peak < 1.0);
        // Bank 7 relies on self-oscillation or noise; everything else has a waveform on.
        CHECK (peak > 1e-4);
    }
}

#include "dsp/chorus/ChorusBoard.h"

TEST_CASE ("chorus: off passes dry identically on both channels; I is stereo; I+II is near mono")
{
    const double sr = 48000.0;
    auto run = [&] (ChorusSwitch mode, std::vector<double>& l, std::vector<double>& r)
    {
        ChorusBoard c;
        c.prepare (cal().chorus, sr);
        c.setMode (mode);
        l.clear(); r.clear();
        for (int i = 0; i < 2 * 48000; ++i)
        {
            const double x = std::sin (2.0 * 3.14159265358979323846 * 440.0 * i / sr);
            double a, b;
            c.process (x, a, b);
            if (i >= 48000) { l.push_back (a); r.push_back (b); }
        }
    };
    std::vector<double> l, r;
    run (ChorusSwitch::off, l, r);
    double diff = 0.0;
    for (std::size_t i = 0; i < l.size(); ++i) diff = std::max (diff, std::abs (l[i] - r[i]));
    CHECK (diff < 1e-12);

    auto stereoWidth = [] (const std::vector<double>& a, const std::vector<double>& b)
    {
        double side = 0.0, mid = 0.0;
        for (std::size_t i = 0; i < a.size(); ++i)
        {
            side += (a[i] - b[i]) * (a[i] - b[i]);
            mid += (a[i] + b[i]) * (a[i] + b[i]);
        }
        return std::sqrt (side / mid);
    };
    run (ChorusSwitch::I, l, r);
    const double wI = stereoWidth (l, r);
    run (ChorusSwitch::I_II, l, r);
    const double wIII = stereoWidth (l, r);
    CHECK (wI > 0.05);
    CHECK (wIII < wI * 0.2);
}

TEST_CASE ("chorus: the BBD input overloads above the 6 Vp-p bias point while the dry path stays linear")
{
    const double sr = 48000.0;
    // Peak of the wet component (output minus the exactly known dry part) relative to the input peak.
    auto wetRatio = [&] (ChorusSwitch mode, double peak)
    {
        ChorusBoard c;
        c.prepare (cal().chorus, sr);
        c.setMode (mode);
        double wetPeak = 0.0;
        for (int i = 0; i < 2 * 48000; ++i)
        {
            const double x = peak * std::sin (2.0 * 3.14159265358979323846 * 1000.0 * i / sr);
            double a, b;
            c.process (x, a, b);
            if (i >= 48000) wetPeak = std::max (wetPeak, std::abs (a - x * cal().chorus.dryGain));
        }
        return wetPeak / peak;
    };
    CHECK (wetRatio (ChorusSwitch::off, 8.0) < 1e-9);         // dry only, exactly linear
    const double quiet = wetRatio (ChorusSwitch::I, 0.5);      // 1 Vp-p
    const double nominal = wetRatio (ChorusSwitch::I, 2.0);    // 4 Vp-p: below the bias point
    const double hot = wetRatio (ChorusSwitch::I, 8.0);        // 16 Vp-p: a loud chord at LEVEL +5
    CHECK_THAT (nominal, WithinRel (quiet, 0.05));
    CHECK (hot < 0.75 * nominal);
}

TEST_CASE ("hold: keys stay latched after release, last six remain, pedal release frees them")
{
    Synth s;
    PanelState p;
    p.sawOn = true; p.vcfFreq = 10.0; p.attack = 0.0; p.decay = 0.0; p.sustain = 10.0; p.release = 0.0;
    p.hold = true;
    s.setPanel (p);
    s.prepare (cal(), 48000.0, 440.0);
    std::vector<float> l (480), r (480);
    std::vector<MidiEvent> ev;
    for (int n = 0; n < 8; ++n) ev.push_back ({ 0, MidiEvent::Type::noteOn, 48 + n, 0.0 });
    for (int n = 0; n < 8; ++n) ev.push_back ({ 1, MidiEvent::Type::noteOff, 48 + n, 0.0 });
    s.render (l.data(), r.data(), 480, ev);
    CHECK (s.activeVoices() == 6);
    p.hold = false;
    s.setPanel (p);
    std::vector<MidiEvent> none;
    for (int b = 0; b < 20; ++b) s.render (l.data(), r.data(), 480, none);
    CHECK (s.activeVoices() == 0);
}

TEST_CASE ("arpeggio: UP over 2 octaves steps through the held chord at the slider rate")
{
    Synth s;
    PanelState p;
    p.sawOn = true; p.vcfFreq = 10.0; p.attack = 0.0; p.decay = 0.0; p.sustain = 10.0; p.release = 0.0;
    p.arpOn = true; p.arpMode = ArpMode::up; p.arpRange = 2; p.arpRate = 10.0; // 50 Hz
    s.setPanel (p);
    s.prepare (cal(), 48000.0, 440.0);
    const int n = 48000;
    std::vector<float> l (n), r (n);
    std::vector<MidiEvent> ev { { 0, MidiEvent::Type::noteOn, 60, 0.0 }, { 0, MidiEvent::Type::noteOn, 64, 0.0 }, { 0, MidiEvent::Type::noteOn, 67, 0.0 } };
    s.render (l.data(), r.data(), n, ev);
    // Count envelope onsets: with A=0 D=0 S=10 R=0 and a 55 % gate at 50 Hz there are ~50 bursts/s.
    // Envelope follower (1 ms RMS windows), count onsets after a quiet window.
    int bursts = 0;
    bool in = false;
    const int win = 48;
    for (int i = 0; i + win <= n; i += win)
    {
        double e = 0.0;
        for (int k = 0; k < win; ++k) e += l[i + k] * l[i + k];
        const bool loud = std::sqrt (e / win) > 1e-3;
        if (loud && ! in) ++bursts;
        in = loud;
    }
    CHECK (bursts >= 40);
    CHECK (bursts <= 60);
    CHECK (s.activeVoices() <= 1);
}

TEST_CASE ("key transpose shifts pitch upward by the chosen interval")
{
    auto freqOf = [] (int keyTranspose)
    {
        Synth s;
        PanelState p;
        p.sawOn = true; p.vcfFreq = 10.0; p.attack = 0.0; p.decay = 0.0; p.sustain = 10.0; p.release = 0.0;
        p.keyTranspose = keyTranspose;
        s.setPanel (p);
        s.prepare (cal(), 48000.0, 440.0);
        std::vector<float> l (48000), r (48000);
        std::vector<MidiEvent> ev { { 0, MidiEvent::Type::noteOn, 69, 0.0 } };
        s.render (l.data(), r.data(), 48000, ev);
        std::vector<double> d (l.begin() + 4800, l.end());
        return zeroCrossingFrequency (d, 48000.0, 0);
    };
    CHECK_THAT (freqOf (0), WithinRel (440.0, 0.01));
    CHECK_THAT (freqOf (7), WithinRel (440.0 * std::exp2 (7.0 / 12.0), 0.01));
}

#include "dsp/presets/PresetFormat.h"

TEST_CASE ("preset JSON round-trips every patch field and rejects bad input")
{
    std::ifstream in (JANE60_FACTORY_PATCHES_FILE);
    std::stringstream ss;
    ss << in.rdbuf();
    const auto patches = parseFactoryPatches (ss.str());
    for (const auto& fp : patches)
    {
        Preset p;
        p.name = fp.name;
        p.bank = "Factory";
        p.tags = { "factory" };
        p.panel = fp.panel;
        const auto text = presetToJson (p);
        const auto back = presetFromJson (text);
        INFO ("patch " << fp.number);
        CHECK (back.name == fp.name);
        CHECK (patchFieldsEqual (back.panel, fp.panel));
    }
    CHECK_THROWS_AS (presetFromJson ("{}"), std::runtime_error);
    CHECK_THROWS_AS (presetFromJson ("{\"schema\":1}"), std::runtime_error);
    CHECK_THROWS_AS (presetFromJson ("not json"), std::runtime_error);

    // Performance fields are not part of a preset.
    PanelState a = patches[0].panel, b = patches[0].panel;
    b.arpOn = true; b.volume = 2.0; b.hold = true;
    CHECK (patchFieldsEqual (a, b));
    b.vcfFreq += 0.5;
    CHECK_FALSE (patchFieldsEqual (a, b));
}

TEST_CASE ("condition layer: the VCF trim choice moves self-oscillation by the calibration offset, presets carry none of it")
{
    // Patch 84 from the Service Notes calibration (FREQ 3, RES 10, no ENV / LFO / KYBD): the
    // filter self-oscillates at 248 Hz on a unit trimmed to the manual.
    PanelState p;
    p.sawOn = false; p.pulseOn = false; p.subOn = false;
    p.vcfFreq = 3.0; p.vcfRes = 10.0; p.vcfEnv = 0.0; p.vcfLfo = 0.0; p.vcfKybd = 0.0;
    p.attack = 0.0; p.decay = 0.0; p.sustain = 10.0; p.release = 0.0;
    p.chorus = ChorusSwitch::off;
    auto oscHz = [&] (const Condition& c)
    {
        Synth s;
        s.setPanel (p);
        s.prepare (cal(), 48000.0, 440.0);
        s.setCondition (c);
        // The oscillation grows from the mixer's noise bleed: give it 4 s, measure the last second.
        std::vector<float> l (48000), r (48000);
        std::vector<MidiEvent> ev { { 0, MidiEvent::Type::noteOn, 60, 0.0 } };
        s.render (l.data(), r.data(), 48000, ev);
        std::vector<MidiEvent> none;
        for (int i = 0; i < 3; ++i) s.render (l.data(), r.data(), 48000, none);
        std::vector<double> x (l.begin(), l.end());
        double peak = 0.0;
        for (double v : x) peak = std::max (peak, std::abs (v));
        INFO ("self-oscillation peak " << peak);
        return zeroCrossingFrequency (x, 48000.0, 0);
    };
    const double manual = oscHz (Condition::serviceNotes());
    const double demo = oscHz (Condition::demoUnit (cal()));
    CHECK_THAT (manual, WithinRel (248.0, 0.05));
    CHECK_THAT (demo / manual, WithinRel (std::exp2 (cal().vcf.trimOffsetOct), 0.05));

    // prepare() starts from the demo unit, as the render tools and the plugin default do.
    Synth s;
    s.setPanel (p);
    s.prepare (cal(), 48000.0, 440.0);
    CHECK (s.condition().vcfTrimOffsetOct == cal().vcf.trimOffsetOct);

    // The preset format has no field for it: a round trip of the panel is unaffected by the condition.
    Preset pr;
    pr.name = "x";
    pr.panel = p;
    CHECK (presetToJson (pr).find ("trim") == std::string::npos);
}

TEST_CASE ("condition layer: drive changes only the filter's saturation, spread makes voices differ, chorus noise can be muted")
{
    // Drive: a small signal passes with the same gain whatever the drive; a full-level saw gains harmonics.
    const double sr = 48000.0;
    auto response = [&] (double driveDb, double amplitudeV)
    {
        Ir3109 f;
        f.configure (cal().vcf, sr);
        f.set (200.0, 0.0); // at the corner the stage inputs see x - y of the full swing
        f.setDrive (std::pow (10.0, driveDb / 20.0));
        std::vector<double> y;
        for (int i = 0; i < 48000; ++i)
            y.push_back (f.process (amplitudeV * std::sin (2.0 * 3.14159265358979323846 * 200.0 * i / sr)));
        double mx = 0.0;
        for (std::size_t i = 24000; i < y.size(); ++i) mx = std::max (mx, std::abs (y[i]));
        return mx;
    };
    CHECK_THAT (response (6.0, 0.05) / response (-6.0, 0.05), WithinRel (1.0, 0.01));   // linear region: no level change
    CHECK (response (6.0, 6.0) / response (-6.0, 6.0) < 0.97);                           // 12 Vp-p: more compression at +6 dB

    // Spread: two voices on the same note differ in level and cutoff under the tolerances, not at spread 0.
    PanelState p;
    p.sawOn = true; p.vcfFreq = 5.0; p.attack = 0.0; p.decay = 0.0; p.sustain = 10.0; p.release = 0.0;
    auto voicePeak = [&] (std::size_t index, double spread)
    {
        Voice v;
        v.configure (cal(), sr);
        v.setPanel (p);
        const Condition::Tolerances tol;
        const auto& u = Condition::pattern[index];
        VoiceDeviation d;
        d.sawScale = 1.0 + spread * tol.sawAmplitude * u[0];
        d.vcaScale = std::pow (10.0, spread * tol.vcaDb * u[4] / 20.0);
        v.setDeviation (d);
        PitchTable pt;
        pt.configure (cal().clock.masterClockHz, cal().clock.tuningA4Hz);
        v.setPitch (pt.divisor[60], cal().clock.masterClockHz, cal().dco.sawVpp);
        v.noteOn (60);
        double mx = 0.0;
        for (int i = 0; i < 9600; ++i) mx = std::max (mx, std::abs (v.tick (0.0, 0.0, 0.0)));
        return mx;
    };
    CHECK_THAT (voicePeak (0, 0.0), WithinRel (voicePeak (1, 0.0), 1e-9));
    CHECK (std::abs (voicePeak (0, 1.0) / voicePeak (1, 1.0) - 1.0) > 0.05);

    // Chorus noise: with no input the wet path carries only the hiss; the condition can mute it.
    auto hiss = [&] (double gain)
    {
        ChorusBoard c;
        c.prepare (cal().chorus, sr);
        c.setMode (ChorusSwitch::I);
        c.setNoiseGain (gain);
        double l, r, acc = 0.0;
        for (int i = 0; i < 48000; ++i) { c.process (0.0, l, r); if (i > 24000) acc += l * l; }
        return std::sqrt (acc / 24000.0);
    };
    CHECK (hiss (1.0) > 1e-6);
    CHECK (hiss (0.0) == 0.0);
    // The key is the hiss at the BBD; the post filters and the wet gain take a few dB off by the output.
    CHECK_THAT (20.0 * std::log10 (hiss (1.0) / 1.4142), WithinAbs (cal().chorus.noiseDbRe4Vpp, 6.0));
}

TEST_CASE ("chorus: the BBD's clock-dependent bandwidth darkens the wet path more at the longest delay")
{
    // Hold the sweep at one end by driving a long sine and measuring the wet component at 8 kHz over
    // a window short against the LFO; compare the LFO's two extremes.
    const double sr = 96000.0;
    auto wetLevelAt = [&] (double seconds)
    {
        ChorusBoard c;
        c.prepare (cal().chorus, sr);
        c.setMode (ChorusSwitch::I);
        c.setNoiseGain (0.0);
        const int n = static_cast<int> (seconds * sr);
        double l = 0.0, r = 0.0, acc = 0.0;
        int count = 0;
        for (int i = 0; i < n; ++i)
        {
            const double x = 0.5 * std::sin (2.0 * 3.14159265358979323846 * 8000.0 * i / sr);
            c.process (x, l, r);
            if (i >= n - 1000) { const double w = l - x * cal().chorus.dryGain; acc += w * w; ++count; }
        }
        return std::sqrt (acc / count);
    };
    // Mode I: 0.513 Hz triangle starting at phase 0 (delay minimum); a quarter period later it is at
    // the maximum. The ratio is what the sinc term predicts: 24 kHz vs 77 kHz clock at 8 kHz.
    const double period = 1.0 / cal().chorus.modes[0].lfoRateHz;
    const double atMin = wetLevelAt (period + 0.002);
    const double atMax = wetLevelAt (period + 0.5 * period + 0.002);
    const double ratioDb = 20.0 * std::log10 (atMax / atMin);
    INFO ("wet 8 kHz level at the longest delay minus the shortest: " << ratioDb << " dB");
    CHECK (ratioDb < -0.8);
    CHECK (ratioDb > -4.0);
}

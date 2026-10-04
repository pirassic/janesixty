// Jane-Sixty: calibration data for the Juno-60 model.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Every hardware constant the DSP uses lives in calibration/juno60.json and is
// loaded into this struct. Each JSON entry carries a "source" tag so the
// calibration report can list where every number came from. DSP code never
// hard-codes a hardware value; it reads it from here.

#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace jane60
{

/// A slider-to-value table sampled at slider positions 0, 1, ..., 10 (11 points).
struct SliderTable
{
    std::array<double, 11> values {};

    /// Linear interpolation between the 11 points. `slider` is 0..10.
    [[nodiscard]] double at (double slider) const noexcept;
};

/// One chorus mode (I, II, or I+II).
struct ChorusMode
{
    std::string name;
    double lfoRateHz = 0.0;
    double delayMinMs = 0.0;
    double delayMaxMs = 0.0;
    bool stereo = true;   ///< true: right channel modulation inverted; false: both in phase
};

struct Calibration
{
    int schema = 0;

    struct Clock
    {
        double masterClockHz = 1902810.0;  ///< Service Notes p.14
        double tuningA4Hz = 442.0;         ///< Service Notes reference; plugin default 440
        double benderMaxCents = 700.0;
        double lfoMaxCents = 300.0;
        double tuneMaxCents = 50.0;
    } clock;

    struct Dco
    {
        double sawVpp = 12.0;              ///< sawtooth amplitude, 0 to -12 V
        double pwmCvAt50Pct = -6.5;        ///< comparator CV for 50 % duty
        double pwmCvAt97Pct = -0.5;        ///< comparator CV for 97 % duty
        double pulseMinDuty = 0.50;
        double pulseMaxDuty = 0.95;        ///< calibration target "PWM 95 %"
        double kcvDacBits = 7;             ///< 7-bit key CV DAC (Service Notes Fig. 2)
        double kcvVoltsPerOctave = 0.48;   ///< DAC output step per octave before anti-log
        double resetCapNf = 1.0;           ///< C7
        double resetPulseCapPf = 270.0;    ///< C6
        double resetPulseResistorOhm = 10000.0; ///< R34
        double noiseVppAtMax = 4.0;        ///< noise at 10, measured at VCA out
    } dco;

    struct Vcf
    {
        double inputResistorOhm = 68000.0; ///< per stage
        double shuntResistorOhm = 560.0;   ///< per stage, to ground
        double stageCapPf = 240.0;         ///< per stage
        double qCompensation = 0.308;      ///< input-side Q compensation (Juno-6 reading)
        double anchorSliderPos = 3.0;      ///< FREQ slider position for the 248 Hz anchor
        double anchorHz = 248.0;           ///< self-oscillation at the anchor
        double selfOscVpp = 4.0;           ///< resonance trim target
        int keyFollowPivotNote = 60;       ///< C4: no cutoff change at KYBD 10
        double keyFollowOctPerOct = 1.0;   ///< at KYBD 10
        double lfoFullDepthOct = 3.3;      ///< +-3.3 oct (40 Hz..5 kHz sweep at FREQ 3.5)
        double envFullPeakHz = 30000.0;    ///< ENV 10 from FREQ 0 reaches ~30 kHz
        double pedalDefaultVolts = 3.0;    ///< VCF CONTROL jack with nothing plugged in
        double passbandLossAtMaxResDb = 7.2; ///< plugin-derived, needs confirmation
    } vcf;

    struct Hpf
    {
        std::array<double, 4> cornerHz { 0.0, 154.0, 339.0, 720.0 }; ///< position 0 is flat
    } hpf;

    struct Vca
    {
        double voiceOutVpp = 4.0;          ///< saw at C4, VCA GAIN target
        double gateRiseMs = 3.0;
        double gateFallMs = 6.0;
    } vca;

    struct Env
    {
        SliderTable attackSeconds;
        SliderTable decaySeconds;
        SliderTable releaseSeconds;
        SliderTable sustainLevel;
        double attackOvershootTarget = 1.58; ///< RC charges toward this, truncated at 1.0
        double decayShapeK = 4.6;            ///< level = S + (1-S) e^(-k t/T)
        double timingCapNf = 47.0;
    } env;

    struct Lfo
    {
        SliderTable rateHz;
        SliderTable delayHoldSeconds;
        SliderTable delayFadeSeconds;
        double dcoLfoGainVpp = 14.0;         ///< LFO GAIN trim target at TP19
    } lfo;

    struct Chorus
    {
        std::vector<ChorusMode> modes;
        int bbdStages = 256;
        double bbdPathGainDb = 2.3;          ///< Holters & Parker measurement
        double dryGain = 0.83;
        double wetGain = 1.0;
    } chorus;

    struct Firmware
    {
        int sliderBits = 8;                  ///< all 16 sliders read by an 8-bit ADC
        double panelLoopMs = 7.2;            ///< Panel Board B program loop
        int voices = 6;
    } firmware;

    /// Source tag per dotted key, e.g. "vcf.anchorHz" -> "service-notes p.24".
    std::map<std::string, std::string> sources;

    /// Parse from JSON text. Throws std::runtime_error with a message on failure.
    static Calibration fromJson (std::string_view jsonText);

    /// Load from a file path. Throws on failure.
    static Calibration fromFile (const std::string& path);

    /// Keys whose source tag is "assumed" (for the calibration report).
    [[nodiscard]] std::vector<std::string> assumedKeys() const;
};

} // namespace jane60

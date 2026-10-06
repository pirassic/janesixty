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
        double stage1DriveDb = 0.0;        ///< level into the OTA pairs relative to a 12 Vp-p input through 68 k / 560 (schematic mixer reading)
        double anchorSliderPos = 3.0;      ///< FREQ slider position for the 248 Hz anchor
        double anchorHz = 248.0;           ///< self-oscillation at the anchor
        double trimOffsetOct = 0.0;        ///< unit trim relative to the Service Notes 248 Hz point (condition layer)
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
        double voiceOutVpp = 4.0;          ///< saw at C4, VCA GAIN target (TP4)
        double sumGainPerVoice = 3.3 / 27.0; ///< IC23 summer: R50 27 k per voice into R369 3.3 k
        double sumToChorusInput = 10.0 / 13.3; ///< R370 3.3 k / R371 10 k divider to TP8 (SIG OUT)
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
        double bbdPathGainDb = 2.3;          ///< Holters & Parker measurement (BBD path before the summer)
        double dryGain = 1.0;                ///< summer: 100 k feedback / 39 k dry, normalised to 1
        double wetGain = 0.83;               ///< summer: 100 k / 47 k wet, relative to dry
        double bbdClipVpp = 6.0;             ///< chorus input level (LEVEL 0) at which the BBD just does not clip
        double bbdClipRoomV = 1.5;           ///< soft region above the knee (assumed)
        double noiseDbRe4Vpp = -84.0;        ///< BBD hiss, rms re a 4 Vp-p sine (datasheet typical)
        bool bbdSincBandwidth = true;        ///< BBD sample-and-hold roll-off tracking the clock (fractional model only)
        int bbdModel = 1;                    ///< 1: Holters & Parker variable-rate model; 0: fractional delay with biquads
        /// Pre-BBD chain: one real pole and two second-order sections (Sallen-Key with emitter followers).
        double preRealHz = 7410.0, preAHz = 9690.0, preAQ = 0.55, preBHz = 10340.0, preBQ = 1.24;
        /// Post-BBD chain per channel.
        double postAHz = 8870.0, postAQ = 0.54, postBHz = 10380.0, postBQ = 1.24, postRealHz = 28000.0;
    } chorus;

    /// Output voicing that has no circuit source yet. Provisional: fitted by ear against
    /// the factory demo recordings and meant to be zeroed once a measurement replaces it.
    struct Voicing
    {
        double lowShelfDb = 0.0;             ///< gain below lowShelfHz, after the chorus
        double lowShelfHz = 150.0;
    } voicing;

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

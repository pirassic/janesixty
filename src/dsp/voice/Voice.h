// SPDX-License-Identifier: GPL-3.0-or-later
// One of six voices: DCO, mixer, IR3109, BA662 VCA, IR3R01 envelope, plus the
// per-voice cutoff CV summing (Service Notes p.15 Fig. 4).
#pragma once

#include "dsp/Calibration.h"
#include "dsp/PanelState.h"
#include "dsp/dco/Dco.h"
#include "dsp/env/Ir3r01Envelope.h"
#include "dsp/vca/Vca.h"
#include "dsp/vcf/Ir3109.h"

#include <cmath>

namespace jane60
{

/// Panel-to-circuit mappings that are not yet pinned by measurement are collected
/// here so they are easy to find and replace (research plan section 1.5).
struct VcfMapping
{
    /// Cutoff slider: octaves per slider unit, anchored at the measured 248 Hz at FREQ 3.
    /// 1.33 = the 13.3-octave cutoff span of the Juno-6 (4 Hz to 40 kHz) and Juno-106
    /// (5 Hz to 50 kHz) specifications over the 10 slider units; it also puts the ENV
    /// full depth at 10.9 octaves, matching the published 10.8 to 11. Confirmed against
    /// the factory demo recording (tools/listen): at 0.9 the plugin sat 10 to 20 dB dark
    /// above 2 kHz on patches with FREQ above 5, at 1.33 within a few dB. Tagged spec-derived.
    double octavesPerSliderUnit = 1.33;

    /// Resonance slider position at which k reaches 4 (self-oscillation threshold).
    /// assumed: plugin-derived 0.75..0.8 of travel.
    double selfOscSliderPos = 7.8;
    /// Feedback gain at RES 10. Fitted so the model self-oscillates at the Service
    /// Notes trim target of 4 Vp-p (adj. 8-1): 4.05 in this model (research: fit, phase 1).
    double kMax = 4.05;
    /// At full feedback the saturating stages oscillate about 8 % below the small-signal
    /// corner in this model; the 248 Hz anchor is a measured self-oscillation frequency,
    /// so the corner is raised to compensate. assumed: replaced by the ngspice reference in phase 4.
    double selfOscShift = 248.0 / 227.9;

    /// Bender and DCO LFO depth sliders: square-law taper (plugin-derived).
    static double depthTaper (double slider0to10) noexcept
    {
        const double x = slider0to10 / 10.0;
        return x * x;
    }

    /// VCF ENV and LFO depth sliders drive BA662 control VCAs (IC23, IC24 on Panel Board A)
    /// whose gain is linear in control current, so the depth is linear in the slider.
    /// Was square-law (plugin-derived); linear makes the envelope click on the organ and
    /// celesta patches as percussive as the originals. schematic reading, confirm in phase 4.
    static double cvDepth (double slider0to10) noexcept { return slider0to10 / 10.0; }
};

class Voice
{
public:
    void configure (const Calibration& cal, double sampleRate) noexcept
    {
        cal_ = &cal;
        sr_ = sampleRate;
        dco_.configure (cal.dco, sampleRate);
        vcf_.configure (cal.vcf, sampleRate);
        vca_.configure (cal.vca, sampleRate);
        env_.configure (cal.env, sampleRate);
        setTrimOffsetOct (cal.vcf.trimOffsetOct); // the demo unit's trim until the condition layer says otherwise
    }

    /// Unit trim (condition layer), octaves relative to the Service Notes 248 Hz point.
    void setTrimOffsetOct (double oct) noexcept
    {
        trimOffsetOct_ = oct;
        // Cutoff at FREQ 0 (used to size the ENV depth so ENV 10 from FREQ 0 peaks at envFullPeakHz).
        const Calibration::Vcf& v = cal_->vcf;
        const double f0 = v.anchorHz * std::exp2 ((0.0 - v.anchorSliderPos) * map_.octavesPerSliderUnit + trimOffsetOct_);
        envFullDepthOct_ = std::log2 (v.envFullPeakHz / f0);
    }

    void noteOn (int note) noexcept
    {
        note_ = note;
        gate_ = true;
        env_.gate (true);
        active_ = true;
    }

    void noteOff() noexcept
    {
        gate_ = false;
        env_.gate (false);
    }

    [[nodiscard]] bool isActive() const noexcept { return active_; }
    [[nodiscard]] bool isGated() const noexcept { return gate_; }
    [[nodiscard]] int note() const noexcept { return note_; }

    /// Per-block control update.
    void setPanel (const PanelState& p) noexcept
    {
        panel_ = &p;
        env_.setSliders (p.attack, p.decay, p.sustain, p.release);
        // Manual pulse width: slider 0 = 50 %, 10 = 95 % (raised-cosine law, plugin-derived).
        const double x = p.dcoPwm / 10.0;
        manualDuty_ = 0.5 + 0.45 * (0.5 - 0.5 * std::cos (3.14159265358979323846 * x));
    }

    /// Pitch for this block from the allocator (divisor at the transposed note) and the clock.
    void setPitch (long divisor, double clockHz, double ampVpp) noexcept
    {
        dco_.setPitch (divisor, clockHz);
        dco_.setAmplitudeVpp (ampVpp);
    }

    /// Render one sample. lfo: -1..1 (delay-scaled), noise: volts, benderVcf: -1..1 scaled by sens.
    double tick (double lfo, double noise, double benderVcfVolts) noexcept
    {
        const PanelState& p = *panel_;

        // Envelope
        const double env = env_.tick();

        // PWM source
        double duty = manualDuty_;
        const double pwmDepth = p.dcoPwm / 10.0;
        if (p.pwmMode == PwmMode::lfo)
            duty = 0.5 + 0.45 * pwmDepth * (0.5 + 0.5 * lfo);
        else if (p.pwmMode == PwmMode::env)
            duty = 0.5 + 0.45 * pwmDepth * env;
        dco_.setDuty (duty);

        // Waveforms and mixer (relative levels plugin-derived: pulse/sub -1.3 dB, noise -1.8 dB)
        const auto w = dco_.tick();
        double mix = 0.0;
        if (p.sawOn) mix += w.saw;
        if (p.pulseOn) mix += w.pulse * 0.861;
        if (p.subOn) mix += w.sub * 0.861 * sliderTaper (p.subLevel);
        mix += noise * 0.813 * sliderTaper (p.noiseLevel);
        // Noise bleed with the slider at 0: the mixer never reaches a mathematically silent
        // input, which is also what lets the VCF self-oscillation start (bank 7 patches).
        // assumed: -100 dB relative to NOISE 10.
        mix += noise * 1e-5;

        // Cutoff CV: FREQ + ENV*depth*polarity + LFO*depth + KYBD + pedal + bender
        const Calibration::Vcf& v = cal_->vcf;
        double oct = (p.vcfFreq - v.anchorSliderPos) * map_.octavesPerSliderUnit + trimOffsetOct_;
        const double envDepth = VcfMapping::cvDepth (p.vcfEnv) * envFullDepthOct_;
        oct += (p.vcfPolarity == VcfPolarity::normal ? 1.0 : -1.0) * envDepth * env;
        oct += VcfMapping::cvDepth (p.vcfLfo) * v.lfoFullDepthOct * lfo;
        oct += (p.vcfKybd / 10.0) * v.keyFollowOctPerOct * (note_ + octaveOffset_ - v.keyFollowPivotNote) / 12.0;
        oct += (benderVcfVolts / 5.0) * 2.0; // assumed: full bender VCF ~ +-2 octaves
        oct += ((p.vcfPedalVolts - v.pedalDefaultVolts) / 5.0) * 2.0;
        const double cutoff = v.anchorHz * std::exp2 (oct);

        // Resonance
        double k = (p.vcfRes / map_.selfOscSliderPos) * 4.0;
        if (k > map_.kMax) k = map_.kMax;
        const double kn = k / map_.kMax;
        const double corner = cutoff * (1.0 + (map_.selfOscShift - 1.0) * kn * kn);
        vcf_.set (corner, k);
        const double filtered = vcf_.process (mix);

        // VCA
        double gain = 0.0;
        if (p.vcaMode == VcaMode::env)
            gain = env;
        else
            gain = vca_.gateControl (gate_);

        // Voice ends when envelope is idle (ENV mode) or gate slew has decayed (GATE mode)
        if (! gate_ && ! env_.active() && ! vca_.gateActive())
            active_ = false;

        // Scale: 12 Vp-p saw through the filter gives 4 Vp-p at the VCA output (Service Notes adj. 5-1)
        return filtered * gain * (cal_->vca.voiceOutVpp / cal_->dco.sawVpp);
    }

    void setOctaveOffset (int semis) noexcept { octaveOffset_ = semis; }

private:
    /// Two-segment taper for the sub and noise sliders (plugin-derived): lower half linear at 0.406.
    static double sliderTaper (double s) noexcept
    {
        const double x = s / 10.0;
        return x < 0.5 ? x * 0.812 : 0.406 + (x - 0.5) * 1.188;
    }

    const Calibration* cal_ = nullptr;
    const PanelState* panel_ = nullptr;
    double sr_ = 48000.0;
    Dco dco_;
    Ir3109 vcf_;
    Ba662Vca vca_;
    Ir3r01Envelope env_;
    VcfMapping map_;
    double envFullDepthOct_ = 10.0;
    double trimOffsetOct_ = 0.0;
    double manualDuty_ = 0.5;
    int note_ = 60;
    int octaveOffset_ = 0;
    bool gate_ = false;
    bool active_ = false;
};

} // namespace jane60

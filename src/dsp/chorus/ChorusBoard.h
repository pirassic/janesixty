// SPDX-License-Identifier: GPL-3.0-or-later
// The Juno-60 chorus board (Service Notes p.11): input gain, a pre-BBD low-pass
// chain, two MN3009 bucket-brigade lines clocked from one triangle LFO (right
// channel inverted in modes I and II, in phase for I+II), post filters, a JFET
// mute with a slow fade, and summers mixing dry and wet into each output.
//
// Phase 2 model: the BBD line is a fractional delay whose length follows the
// mode's measured delay sweep (research/01 section 8), with the pre and post
// filter chains as cascaded biquads from the schematic-derived pole sets, a
// first-order transfer-loss term vs clock, and the measured no-compander noise
// floor as a constant hiss. The Holters-Parker variable-rate model and the clock
// residue arrive in phase 4.
#pragma once

#include "dsp/Calibration.h"
#include "dsp/PanelState.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace jane60
{

/// Second-order section, TPT state-variable form, low-pass output.
class Biquad2 final
{
public:
    void set (double f0, double q, double sampleRate) noexcept
    {
        constexpr double pi = 3.14159265358979323846;
        g_ = std::tan (pi * f0 / sampleRate);
        k_ = 1.0 / q;
        a1_ = 1.0 / (1.0 + g_ * (g_ + k_));
        a2_ = g_ * a1_;
        a3_ = g_ * a2_;
        s1_ = s2_ = 0.0;
    }
    double process (double x) noexcept
    {
        const double v3 = x - s2_;
        const double v1 = a1_ * s1_ + a2_ * v3;
        const double v2 = s2_ + a2_ * s1_ + a3_ * v3;
        s1_ = 2.0 * v1 - s1_;
        s2_ = 2.0 * v2 - s2_;
        return v2;
    }
private:
    double g_ = 0, k_ = 1, a1_ = 0, a2_ = 0, a3_ = 0, s1_ = 0, s2_ = 0;
};

class OnePoleLp final
{
public:
    void set (double fc, double sampleRate) noexcept
    {
        constexpr double pi = 3.14159265358979323846;
        sr_ = sampleRate;
        const double g = std::tan (pi * fc / sampleRate);
        G_ = g / (1.0 + g);
        s_ = 0.0;
    }
    double process (double x) noexcept
    {
        const double v = (x - s_) * G_;
        const double y = v + s_;
        s_ = y + v;
        return y;
    }
    /// One sample with a new corner (TPT form, stable under modulation). Corners above 0.45 fs
    /// are held there, where the pole is effectively out of band.
    double processTracking (double x, double fc) noexcept
    {
        constexpr double pi = 3.14159265358979323846;
        const double g = std::tan (pi * std::min (fc, sr_ * 0.45) / sr_);
        G_ = g / (1.0 + g);
        return process (x);
    }
private:
    double G_ = 0, s_ = 0;
    double sr_ = 48000.0;
};

/// One MN3009 line: fractional delay with transfer loss that grows as the clock slows.
class BbdLine final
{
public:
    void prepare (double sampleRate, double maxDelayMs) noexcept
    {
        sr_ = sampleRate;
        const auto len = static_cast<std::size_t> (maxDelayMs * 1e-3 * sampleRate) + 8;
        buf_.assign (len, 0.0);
        w_ = 0;
    }
    double process (double x, double delayMs) noexcept
    {
        buf_[w_] = x;
        const double d = delayMs * 1e-3 * sr_;
        double r = static_cast<double> (w_) - d;
        const auto n = static_cast<double> (buf_.size());
        while (r < 0.0) r += n;
        const auto i0 = static_cast<std::size_t> (r);
        const double frac = r - static_cast<double> (i0);
        const std::size_t i1 = (i0 + 1) % buf_.size();
        const double y = buf_[i0] + (buf_[i1] - buf_[i0]) * frac;
        w_ = (w_ + 1) % buf_.size();
        return y;
    }
private:
    double sr_ = 48000.0;
    std::vector<double> buf_;
    std::size_t w_ = 0;
};

class ChorusBoard final
{
public:
    void prepare (const Calibration::Chorus& c, double sampleRate) noexcept
    {
        cal_ = c;
        sr_ = sampleRate;
        double maxDelay = 1.0;
        for (const auto& m : c.modes) maxDelay = std::max (maxDelay, m.delayMaxMs);
        lineL_.prepare (sampleRate, maxDelay + 1.0);
        lineR_.prepare (sampleRate, maxDelay + 1.0);

        // Pre- and post-BBD filter chains from the calibration file (sources there).
        preReal_.set (std::min (c.preRealHz, sampleRate * 0.45), sampleRate);
        preA_.set (std::min (c.preAHz, sampleRate * 0.45), c.preAQ, sampleRate);
        preB_.set (std::min (c.preBHz, sampleRate * 0.45), c.preBQ, sampleRate);
        for (std::size_t ch = 0; ch < 2; ++ch)
        {
            postA_[ch].set (std::min (c.postAHz, sampleRate * 0.45), c.postAQ, sampleRate);
            postB_[ch].set (std::min (c.postBHz, sampleRate * 0.45), c.postBQ, sampleRate);
            postReal_[ch].set (std::min (c.postRealHz, sampleRate * 0.45), sampleRate);
        }
        // Mute fade: the JFET gate RC on the schematic (2.2 uF with 150 k / 560 k) gives
        // a slow on/off; assumed ~150 ms.
        fadeCoef_ = 1.0 - std::exp (-1.0 / (0.15 * sampleRate));
        wetGain_ = std::pow (10.0, c.bbdPathGainDb / 20.0);
        // Hiss: uniform noise (rms 1/sqrt 12) scaled to noiseDbRe4Vpp relative to a 4 Vp-p sine (1.414 V rms).
        noiseAmp_ = 1.4142 * std::pow (10.0, c.noiseDbRe4Vpp / 20.0) * std::sqrt (12.0);
        clipKnee_ = 0.5 * c.bbdClipVpp;
        clipRoom_ = c.bbdClipRoomV;
        stages_ = static_cast<double> (c.bbdStages);
        sincL_.set (20000.0, sampleRate);
        sincR_.set (20000.0, sampleRate);
        phase_ = 0.0;
        mute_ = 0.0;
        mode_ = ChorusSwitch::off;
        noiseState_ = 0x2545F491u;
    }

    void setMode (ChorusSwitch m) noexcept { mode_ = m; }

    /// Condition layer: hiss relative to the calibrated level (0 mutes it).
    void setNoiseGain (double linearGain) noexcept { noiseGain_ = linearGain; }

    /// Mono input (volts), stereo output.
    void process (double in, double& outL, double& outR) noexcept
    {
        // Mute target: 1 when a chorus mode is on.
        const double target = mode_ == ChorusSwitch::off ? 0.0 : 1.0;
        mute_ += (target - mute_) * fadeCoef_;

        if (mute_ < 1e-4)
        {
            outL = outR = in * cal_.dryGain;
            return;
        }

        const ChorusMode& m = modeData();
        // Triangle LFO, free-running at the mode's rate.
        phase_ += m.lfoRateHz / sr_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        const double tri = phase_ < 0.5 ? 4.0 * phase_ - 1.0 : 3.0 - 4.0 * phase_; // -1..1
        const double mid = 0.5 * (m.delayMinMs + m.delayMaxMs);
        const double half = 0.5 * (m.delayMaxMs - m.delayMinMs);
        const double dL = mid + half * tri;
        const double dR = m.stereo ? mid - half * tri : dL;

        // Pre-filter (shared), BBD lines. The MN3009 input has no compander: the bias trim leaves
        // 6 Vp-p (LEVEL 0) just clean (datasheet: THD 2.5 % at 1.5 Vrms, a wall near 2 Vrms), so hot
        // patches and big chords overload the wet path while the dry path stays linear.
        const double pre = bbdInput (preReal_.process (preB_.process (preA_.process (in))));
        double wL = lineL_.process (pre, dL);
        double wR = lineR_.process (pre, dR);
        // The BBD output holds each sample for one clock period (Holters & Parker): a sinc(f / fBBD)
        // roll-off with fBBD = stages / (2 delay), 24 to 77 kHz over the sweep. A one-pole tracking
        // the sinc's -3 dB point (0.443 fBBD) stands in for it below the host Nyquist.
        if (cal_.bbdSincBandwidth)
        {
            wL = sincL_.processTracking (wL, 0.443 * stages_ / (2.0 * dL * 1e-3));
            wR = sincR_.processTracking (wR, 0.443 * stages_ / (2.0 * dR * 1e-3));
        }

        // No compander: a constant hiss from the BBDs (calibration chorus.noiseDbRe4Vpp, assumed).
        const double hiss = noiseAmp_ * noiseGain_;
        wL += noise() * hiss;
        wR += noise() * hiss;

        // Post filters, wet gain, mute fade, summers.
        wL = postReal_[0].process (postB_[0].process (postA_[0].process (wL))) * wetGain_ * mute_;
        wR = postReal_[1].process (postB_[1].process (postA_[1].process (wR))) * wetGain_ * mute_;
        outL = in * cal_.dryGain + wL * cal_.wetGain;
        outR = in * cal_.dryGain + wR * cal_.wetGain;
    }

private:
    const ChorusMode& modeData() const noexcept
    {
        const std::size_t idx = mode_ == ChorusSwitch::I ? 0 : mode_ == ChorusSwitch::II ? 1 : 2;
        return cal_.modes[idx < cal_.modes.size() ? idx : 0];
    }
    /// Soft overload of the BBD input: linear to the knee, tanh above it.
    double bbdInput (double x) const noexcept
    {
        const double a = std::abs (x);
        if (a <= clipKnee_) return x;
        const double y = clipKnee_ + clipRoom_ * std::tanh ((a - clipKnee_) / clipRoom_);
        return x < 0.0 ? -y : y;
    }
    double noise() noexcept
    {
        noiseState_ ^= noiseState_ << 13;
        noiseState_ ^= noiseState_ >> 17;
        noiseState_ ^= noiseState_ << 5;
        return static_cast<double> (noiseState_) / 4294967296.0 - 0.5;
    }

    Calibration::Chorus cal_ {};
    double sr_ = 48000.0;
    BbdLine lineL_, lineR_;
    OnePoleLp preReal_;
    Biquad2 preA_, preB_;
    std::array<Biquad2, 2> postA_ {}, postB_ {};
    std::array<OnePoleLp, 2> postReal_ {};
    OnePoleLp sincL_, sincR_;
    double stages_ = 256.0;
    double fadeCoef_ = 0.0, mute_ = 0.0, wetGain_ = 1.0, phase_ = 0.0;
    double noiseAmp_ = 0.0, noiseGain_ = 1.0;
    double clipKnee_ = 3.0, clipRoom_ = 1.5;
    ChorusSwitch mode_ = ChorusSwitch::off;
    unsigned int noiseState_ = 0x2545F491u;
};

} // namespace jane60

// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsp/Synth.h"

#include <algorithm>
#include <cmath>

namespace jane60
{

void Synth::prepare (const Calibration& cal, double sampleRate, double a4Hz)
{
    cal_ = &cal;
    sr_ = sampleRate;
    clock_.configure (cal.clock, a4Hz);
    pitch_.configure (cal.clock.masterClockHz, cal.clock.tuningA4Hz); // PROM table at the nominal clock
    lfo_.configure (cal.lfo, sampleRate);
    // The noise trim target (4 Vp-p at the voice VCA output, adj. 6) is measured after the
    // filter and VCA like the sawtooth's 4 Vp-p (adj. 5-1), so at the mixer the noise sits at
    // the same level as the 12 Vp-p saw.
    noise_.configure (sampleRate, cal.dco.noiseVppAtMax * (cal.dco.sawVpp / cal.vca.voiceOutVpp));
    hpf_.configure (cal.hpf, sampleRate);
    chorus_.prepare (cal.chorus, sampleRate);
    shelfL_.set (cal.voicing.lowShelfHz, sampleRate);
    shelfR_.set (cal.voicing.lowShelfHz, sampleRate);
    shelfGainMinusOne_ = std::pow (10.0, cal.voicing.lowShelfDb / 20.0) - 1.0;
    sumGain_ = cal.vca.sumGainPerVoice * cal.vca.sumToChorusInput;
    for (auto& v : voices_)
    {
        v.configure (cal, sampleRate);
        v.setPanel (panel_);
    }
    setCondition (Condition::demoUnit (cal));
    nextVoice_ = 0;
    physical_.clear();
    latched_.clear();
    arpPattern_.clear();
    arpIndex_ = 0;
    arpPhase_ = 0.0;
    arpSounding_ = -1;
    pedal_ = false;
    lastArpOn_ = panel_.arpOn;
    lastHold_ = panel_.hold;
    physical_.reserve (64);
    latched_.reserve (64);
    arpPattern_.reserve (64);
}

void Synth::setCondition (const Condition& c) noexcept
{
    condition_ = c;
    if (cal_ == nullptr) return;
    const Condition::Tolerances tol;
    const double drive = std::pow (10.0, c.vcfDriveDb / 20.0);
    for (std::size_t i = 0; i < voices_.size(); ++i)
    {
        const auto& u = Condition::pattern[i];
        VoiceDeviation d;
        d.sawScale = 1.0 + c.voiceSpread * tol.sawAmplitude * u[0];
        d.cutoffOct = c.voiceSpread * tol.cutoffOct * u[1];
        d.resonanceScale = 1.0 + c.voiceSpread * tol.resonance * u[2];
        d.envTimeScale = 1.0 + c.voiceSpread * tol.envTime * u[3];
        d.vcaScale = std::pow (10.0, c.voiceSpread * tol.vcaDb * u[4] / 20.0);
        voices_[i].setTrimOffsetOct (c.vcfTrimOffsetOct);
        voices_[i].setDeviation (d);
        voices_[i].setDrive (drive);
    }
    chorus_.setNoiseGain (c.chorusNoise ? std::pow (10.0, c.chorusNoiseDb / 20.0) : 0.0);
}

int Synth::transposeSemis() const noexcept
{
    int t = panel_.keyTranspose;
    switch (panel_.octave)
    {
        case OctaveTranspose::down: t -= 12; break;
        case OctaveTranspose::up: t += 12; break;
        default: break;
    }
    return t;
}

// ---------------------------------------------------------------------------
// Voice level: rotary assignment, the 7th key steals the first (Service Notes p.14).
// ---------------------------------------------------------------------------
void Synth::voiceOn (int note, double velocity, int channel) noexcept
{
    Voice& v = voices_[static_cast<std::size_t> (nextVoice_)];
    nextVoice_ = (nextVoice_ + 1) % kVoices;
    v.noteOn (note, channel,
              Extras::toVca (extras_.velocityTo) ? extras_.velocityScale (velocity) : 1.0,
              Extras::toVcf (extras_.velocityTo) ? extras_.velocityScale (velocity) : 1.0);
}

void Synth::voiceOff (int note) noexcept
{
    for (auto& v : voices_)
        if (v.isGated() && v.note() == note)
            v.noteOff();
}

// ---------------------------------------------------------------------------
// Keyboard level: physical keys, HOLD latching, arpeggio pattern.
// ---------------------------------------------------------------------------
void Synth::keyDown (int note, double velocity, int channel) noexcept
{
    const bool phraseStart = latched_.empty();
    if (std::find (physical_.begin(), physical_.end(), note) == physical_.end())
        physical_.push_back (note);

    if (std::find (latched_.begin(), latched_.end(), note) == latched_.end())
    {
        latched_.push_back (note);
        // HOLD keeps the last six keys (Owner's Manual p.20).
        if (holdActive() && latched_.size() > static_cast<std::size_t> (kVoices))
        {
            const int dropped = latched_.front();
            latched_.erase (latched_.begin());
            if (! panel_.arpOn) voiceOff (dropped);
        }
        if (! panel_.arpOn)
            voiceOn (note, velocity, channel);
    }
    if (phraseStart)
        lfo_.phraseStart();
    arpDirty_ = true;
}

void Synth::keyUp (int note) noexcept
{
    physical_.erase (std::remove (physical_.begin(), physical_.end(), note), physical_.end());
    if (holdActive())
        return; // stays latched
    latched_.erase (std::remove (latched_.begin(), latched_.end(), note), latched_.end());
    if (! panel_.arpOn)
        voiceOff (note);
    arpDirty_ = true;
}

void Synth::releaseUnheld() noexcept
{
    for (auto it = latched_.begin(); it != latched_.end();)
    {
        if (std::find (physical_.begin(), physical_.end(), *it) == physical_.end())
        {
            if (! panel_.arpOn) voiceOff (*it);
            it = latched_.erase (it);
        }
        else
            ++it;
    }
    arpDirty_ = true;
}

void Synth::rebuildArpPattern() noexcept
{
    arpPattern_.clear();
    if (latched_.empty()) return;
    std::vector<int> base (latched_);
    std::sort (base.begin(), base.end());
    base.erase (std::unique (base.begin(), base.end()), base.end());

    // Octaves above the played notes; beyond the keyboard the highest octave repeats
    // (Owner's Manual p.19).
    std::vector<int> up;
    for (int oct = 0; oct < panel_.arpRange; ++oct)
        for (int n : base)
        {
            int m = n + 12 * oct;
            while (m > 108) m -= 12;
            up.push_back (m);
        }

    switch (panel_.arpMode)
    {
        case ArpMode::up:
            arpPattern_ = up;
            break;
        case ArpMode::down:
            arpPattern_.assign (up.rbegin(), up.rend());
            break;
        case ArpMode::upDown:
            arpPattern_ = up;
            for (std::size_t i = up.size() >= 2 ? up.size() - 2 : 0; i > 0; --i)
                arpPattern_.push_back (up[i]);
            break;
    }
    if (arpIndex_ >= arpPattern_.size())
        arpIndex_ = 0;
}

void Synth::arpTick() noexcept
{
    if (arpSounding_ >= 0) { voiceOff (arpSounding_); arpSounding_ = -1; }
    if (arpPattern_.empty()) return;
    arpSounding_ = arpPattern_[arpIndex_];
    voiceOn (arpSounding_);
    arpIndex_ = (arpIndex_ + 1) % arpPattern_.size();
}

void Synth::handle (const MidiEvent& e) noexcept
{
    switch (e.type)
    {
        case MidiEvent::Type::noteOn: keyDown (e.note, e.velocity, e.channel); break;
        case MidiEvent::Type::noteOff: keyUp (e.note); break;
        case MidiEvent::Type::pitchBend:
            // MPE: a member channel's bend belongs to the notes on that channel; the master
            // channel (1) and plain MIDI drive the bender as on the hardware.
            if (extras_.mpeOn && e.channel != 1)
            {
                for (auto& v : voices_)
                    if (v.isActive() && v.channel() == e.channel)
                        v.setNoteBendSemis (e.value * extras_.mpeBendRangeSemis);
            }
            else
                bender_ = e.value;
            break;
        case MidiEvent::Type::channelPressure:
            if (extras_.mpeOn && e.channel != 1)
            {
                const double s = extras_.pressureScale (e.value);
                for (auto& v : voices_)
                    if (v.isActive() && v.channel() == e.channel)
                        v.setPressure (Extras::toVca (extras_.pressureTo) ? s : 1.0, Extras::toVcf (extras_.pressureTo) ? s : 1.0);
            }
            break;
        case MidiEvent::Type::allNotesOff:
            for (auto& v : voices_) v.noteOff();
            physical_.clear();
            latched_.clear();
            arpSounding_ = -1;
            arpDirty_ = true;
            break;
        case MidiEvent::Type::lfoTrig: lfo_.trigButton (e.value > 0.5); break;
        case MidiEvent::Type::holdPedal:
        {
            const bool was = holdActive();
            pedal_ = e.value > 0.5;
            if (was && ! holdActive()) releaseUnheld();
            break;
        }
    }
}

void Synth::updateControls() noexcept
{
    lfo_.setSliders (panel_.lfoRate, panel_.lfoDelay);
    lfo_.setTrigMode (panel_.lfoTrig == LfoTrigMode::automatic ? Lfo::TrigMode::automatic : Lfo::TrigMode::manual);
    hpf_.setPosition (panel_.hpf);
    chorus_.setMode (panel_.chorus);
    const int semis = transposeSemis();
    for (auto& v : voices_)
    {
        v.setPanel (panel_);
        v.setOctaveOffset (semis);
    }

    // HOLD button edge: off releases every key not physically held.
    if (lastHold_ && ! panel_.hold && ! pedal_)
        releaseUnheld();
    lastHold_ = panel_.hold;

    // ARPEGGIO switch edge: on takes over the held keys; off re-sounds them as a chord.
    if (panel_.arpOn != lastArpOn_)
    {
        if (panel_.arpOn)
        {
            for (int n : latched_) voiceOff (n);
            arpIndex_ = 0;
            arpPhase_ = 0.0;
        }
        else
        {
            if (arpSounding_ >= 0) { voiceOff (arpSounding_); arpSounding_ = -1; }
            for (int n : latched_) voiceOn (n);
        }
        lastArpOn_ = panel_.arpOn;
        arpDirty_ = true;
    }
    if (arpDirty_)
    {
        rebuildArpPattern();
        arpDirty_ = false;
    }
}

void Synth::render (float* left, float* right, int numSamples, const std::vector<MidiEvent>& events)
{
    updateControls();

    const double benderDepth = VcfMapping::depthTaper (panel_.benderDco);
    const double dcoLfoDepth = VcfMapping::depthTaper (panel_.dcoLfo);
    const double levelGain = levelSliderGain (panel_.vcaLevel);
    const double volume = (panel_.volume / 10.0) * (panel_.volume / 10.0);
    const double benderVcfVolts = bender_ * (panel_.benderVcf / 10.0) * 5.0;
    const int semis = transposeSemis();

    // Arpeggio clock: 1.5 to 50 Hz over the slider, log taper (assumed).
    const double arpHz = 1.5 * std::pow (50.0 / 1.5, panel_.arpRate / 10.0);
    const double arpInc = arpHz / sr_;
    constexpr double arpGate = 0.55; // step gate fraction (plugin-derived)

    std::size_t ev = 0;
    for (int i = 0; i < numSamples; ++i)
    {
        while (ev < events.size() && events[ev].sampleOffset <= i)
        {
            handle (events[ev++]);
            if (arpDirty_) { rebuildArpPattern(); arpDirty_ = false; }
        }

        if (panel_.arpOn)
        {
            if (arpPhase_ == 0.0) arpTick();
            arpPhase_ += arpInc;
            if (arpSounding_ >= 0 && arpPhase_ >= arpGate)
            {
                voiceOff (arpSounding_);
                arpSounding_ = -1;
            }
            if (arpPhase_ >= 1.0) arpPhase_ = 0.0;
        }

        const double lfo = lfo_.tick();
        const double nz = noise_.tick();

        clock_.update (bender_, benderDepth, lfo, dcoLfoDepth, panel_.tune);
        const double clockHz = clock_.hz();

        double sum = 0.0;
        for (auto& v : voices_)
        {
            if (! v.isActive()) continue;
            int n = v.note() + semis;
            if (n < 0) n = 0;
            if (n > 127) n = 127;
            v.setPitch (pitch_.divisor[n], clockHz, cal_->dco.sawVpp);
            sum += v.tick (lfo, nz, benderVcfVolts);
        }

        // Voice summer and the divider to SIG OUT: the chorus board (HPF, LEVEL VCA, BBDs)
        // works in volts at TP8, which is where its 6 Vp-p bias point is defined.
        const double pre = hpf_.process (sum * sumGain_) * levelGain;
        double oL, oR;
        chorus_.process (pre, oL, oR);
        if (shelfGainMinusOne_ != 0.0)
        {
            oL += shelfGainMinusOne_ * shelfL_.process (oL);
            oR += shelfGainMinusOne_ * shelfR_.process (oR);
        }
        // Master gain: a digital scale factor, not a component. 0.22 puts digital full scale above
        // the chorus board's BBD overload point, so the safety stage below can only engage once the
        // modelled overload is already audible (a 6-note chord at LEVEL +5 with chorus on). The
        // earlier 0.49 clipped Mellow Piano chords with chorus I at LEVEL +2 and VOLUME 10 while
        // the BBD input sat 4 dB under its knee. Listening notes 2026-10-05.
        const double g = 0.22 * volume;
        left[i] = static_cast<float> (outputStage (oL * g));
        if (right != nullptr) right[i] = static_cast<float> (outputStage (oR * g));
    }
}

double Synth::outputStage (double x) noexcept
{
    // Safety stage only (not a modelled component): linear to -1 dBFS, then a soft tanh
    // so the output never exceeds full scale. The hardware's real overload path is the
    // chorus board (no compander), which phase 4 models in its place, before the chorus.
    constexpr double knee = 0.9, room = 1.0 - knee;
    const double a = std::abs (x);
    if (a <= knee) return x;
    const double y = knee + room * std::tanh ((a - knee) / room);
    return x < 0.0 ? -y : y;
}

int Synth::activeVoices() const noexcept
{
    return static_cast<int> (std::count_if (voices_.begin(), voices_.end(), [] (const Voice& v) { return v.isActive(); }));
}

} // namespace jane60

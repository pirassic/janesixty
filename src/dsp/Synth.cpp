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
    noise_.configure (sampleRate, cal.dco.noiseVppAtMax);
    hpf_.configure (cal.hpf, sampleRate);
    for (auto& v : voices_)
    {
        v.configure (cal, sampleRate);
        v.setPanel (panel_);
    }
    nextVoice_ = 0;
    heldKeys_ = 0;
}

int Synth::transposeSemis() const noexcept
{
    switch (panel_.octave)
    {
        case OctaveTranspose::down: return -12;
        case OctaveTranspose::up: return 12;
        default: return 0;
    }
}

void Synth::noteOn (int note) noexcept
{
    // Rotary (cyclic) assignment: the next channel in order, the 7th key steals the first.
    // Service Notes p.14 "Key assignment" and p.22 "UP & DOWN (ROTARY)".
    Voice& v = voices_[static_cast<std::size_t> (nextVoice_)];
    nextVoice_ = (nextVoice_ + 1) % kVoices;
    v.noteOn (note);
    if (heldKeys_ == 0)
        lfo_.phraseStart();
    ++heldKeys_;
}

void Synth::noteOff (int note) noexcept
{
    for (auto& v : voices_)
        if (v.isGated() && v.note() == note)
            v.noteOff();
    if (heldKeys_ > 0) --heldKeys_;
}

void Synth::handle (const MidiEvent& e) noexcept
{
    switch (e.type)
    {
        case MidiEvent::Type::noteOn: noteOn (e.note); break;
        case MidiEvent::Type::noteOff: noteOff (e.note); break;
        case MidiEvent::Type::pitchBend: bender_ = e.value; break;
        case MidiEvent::Type::allNotesOff:
            for (auto& v : voices_) v.noteOff();
            heldKeys_ = 0;
            break;
        case MidiEvent::Type::lfoTrig: lfo_.trigButton (e.value > 0.5); break;
    }
}

void Synth::updateControls() noexcept
{
    lfo_.setSliders (panel_.lfoRate, panel_.lfoDelay);
    lfo_.setTrigMode (panel_.lfoTrig == LfoTrigMode::automatic ? Lfo::TrigMode::automatic : Lfo::TrigMode::manual);
    hpf_.setPosition (panel_.hpf);
    const int semis = transposeSemis();
    for (auto& v : voices_)
    {
        v.setPanel (panel_);
        v.setOctaveOffset (semis);
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

    std::size_t ev = 0;
    for (int i = 0; i < numSamples; ++i)
    {
        while (ev < events.size() && events[ev].sampleOffset <= i)
            handle (events[ev++]);

        // Shared modulators
        const double lfo = lfo_.tick();
        const double nz = noise_.tick();

        // Master clock follows bender, LFO and tune every sample (varicap, continuous).
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

        double out = hpf_.process (sum) * levelGain;
        // Headroom: voice sum is in volts (4 Vp-p per voice nominal). Scale so one voice at
        // LEVEL 0 and VOLUME 10 sits near -16 dBFS.
        out *= 0.08 * volume;
        const float f = static_cast<float> (out);
        left[i] = f;
        if (right != nullptr) right[i] = f;
    }
    // Any events after the last sample (should not happen) are dropped.
}

int Synth::activeVoices() const noexcept
{
    return static_cast<int> (std::count_if (voices_.begin(), voices_.end(), [] (const Voice& v) { return v.isActive(); }));
}

} // namespace jane60

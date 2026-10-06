// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginProcessor.h"
#include "Parameters.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace jane60
{

Calibration Jane60Processor::loadEmbeddedCalibration()
{
    const std::string_view text (BinaryData::juno60_json, static_cast<std::size_t> (BinaryData::juno60_jsonSize));
    return Calibration::fromJson (text);
}

std::vector<FactoryPatch> Jane60Processor::loadEmbeddedFactoryPatches()
{
    const std::string_view text (BinaryData::factory_patches_csv,
                                 static_cast<std::size_t> (BinaryData::factory_patches_csvSize));
    return parseFactoryPatches (text);
}

Jane60Processor::Jane60Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      calibration_ (loadEmbeddedCalibration()),
      factory_ (loadEmbeddedFactoryPatches()),
      apvts_ (*this, &undo_, "JANE60", params::createLayout()),
      presets_ (apvts_, factory_)
{
    events_.reserve (1024);
}

void Jane60Processor::prepareToPlay (double sampleRate, int)
{
    synth_.setPanel (params::readPanel (apvts_));
    synth_.prepare (calibration_, sampleRate, 440.0);
    applyCondition (true);
    applyExtras (true);
}

void Jane60Processor::applyExtras (bool force) noexcept
{
    Extras e;
    const int vd = velocityTo_.load();
    e.velocityOn = vd != 0;
    e.velocityTo = static_cast<Extras::Destination> (vd == 0 ? 3 : vd);
    e.velocityAmount = velocityAmount_.load();
    e.velocitySoft = velocitySoft_.load();
    e.mpeOn = mpe_.load();
    e.pressureTo = static_cast<Extras::Destination> (pressureTo_.load());
    if (! force && extrasApplied_
        && e.velocityOn == appliedExtras_.velocityOn && e.velocityTo == appliedExtras_.velocityTo
        && std::abs (e.velocityAmount - appliedExtras_.velocityAmount) < 1e-9 && e.velocitySoft == appliedExtras_.velocitySoft
        && e.mpeOn == appliedExtras_.mpeOn && e.pressureTo == appliedExtras_.pressureTo)
        return;
    appliedExtras_ = e;
    extrasApplied_ = true;
    synth_.setExtras (e);
}

void Jane60Processor::applyCondition (bool force) noexcept
{
    Condition c = demoTrim_.load() ? Condition::demoUnit (calibration_) : Condition::serviceNotes();
    c.voiceSpread = voiceSpread_.load();
    c.vcfDriveDb = vcfDriveDb_.load();
    const double noiseDb = chorusNoiseDb_.load();
    c.chorusNoise = noiseDb > -90.0;
    c.chorusNoiseDb = c.chorusNoise ? noiseDb : 0.0;
    auto same = [] (double a, double b) { return std::abs (a - b) < 1e-9; };
    if (! force && conditionApplied_
        && same (c.vcfTrimOffsetOct, applied_.vcfTrimOffsetOct) && same (c.voiceSpread, applied_.voiceSpread)
        && same (c.vcfDriveDb, applied_.vcfDriveDb) && c.chorusNoise == applied_.chorusNoise && same (c.chorusNoiseDb, applied_.chorusNoiseDb))
        return;
    applied_ = c;
    conditionApplied_ = true;
    synth_.setCondition (c);
}

bool Jane60Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void Jane60Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // Merge the editor's on-screen / computer keyboard into the host MIDI.
    keyboardState_.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    events_.clear();
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        MidiEvent e;
        e.sampleOffset = meta.samplePosition;
        e.channel = m.getChannel() > 0 ? m.getChannel() : 1;
        if (m.isNoteOn())
        {
            e.type = MidiEvent::Type::noteOn;
            e.note = m.getNoteNumber();
            e.velocity = m.getFloatVelocity();
        }
        else if (m.isNoteOff())
        {
            e.type = MidiEvent::Type::noteOff;
            e.note = m.getNoteNumber();
        }
        else if (m.isPitchWheel())
        {
            e.type = MidiEvent::Type::pitchBend;
            e.value = (m.getPitchWheelValue() - 8192) / 8192.0;
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            e.type = MidiEvent::Type::allNotesOff;
        }
        else if (m.isController() && m.getControllerNumber() == 64)
        {
            // Sustain pedal = the PEDAL HOLD jack.
            e.type = MidiEvent::Type::holdPedal;
            e.value = m.getControllerValue() >= 64 ? 1.0 : 0.0;
        }
        else if (m.isChannelPressure())
        {
            // MPE per-note pressure (member channels); ignored by the synth unless MPE is on.
            e.type = MidiEvent::Type::channelPressure;
            e.value = m.getChannelPressureValue() / 127.0;
        }
        else
            continue;
        if (events_.size() < events_.capacity())
            events_.push_back (e);
    }

    if (uiBenderDirty_.exchange (false))
        events_.push_back ({ 0, MidiEvent::Type::pitchBend, 0, uiBender_.load() });
    if (uiLfoTrigDirty_.exchange (false))
        events_.push_back ({ 0, MidiEvent::Type::lfoTrig, 0, uiLfoTrig_.load() ? 1.0 : 0.0 });
    std::stable_sort (events_.begin(), events_.end(), [] (const MidiEvent& a, const MidiEvent& b) { return a.sampleOffset < b.sampleOffset; });

    synth_.setPanel (params::readPanel (apvts_));
    applyCondition (false);
    applyExtras (false);

    const int n = buffer.getNumSamples();
    float* left = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    synth_.render (left, right, n, events_);
}

juce::AudioProcessorEditor* Jane60Processor::createEditor()
{
    return new Jane60Editor (*this);
}

void Jane60Processor::setCurrentProgram (int index)
{
    if (index < 0 || index >= static_cast<int> (factory_.size()))
        return;
    currentProgram_ = index;
    presets_.load (index); // factory entries are the first 56 browser entries
}

const juce::String Jane60Processor::getProgramName (int index)
{
    if (index < 0 || index >= static_cast<int> (factory_.size()))
        return {};
    const auto& p = factory_[static_cast<std::size_t> (index)];
    return juce::String (p.number) + " " + p.name;
}

void Jane60Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts_.copyState();
    state.setProperty ("program", currentProgram_, nullptr);
    state.setProperty ("demoTrim", demoTrim_.load(), nullptr);
    state.setProperty ("voiceSpread", voiceSpread_.load(), nullptr);
    state.setProperty ("vcfDriveDb", vcfDriveDb_.load(), nullptr);
    state.setProperty ("chorusNoiseDb", chorusNoiseDb_.load(), nullptr);
    state.setProperty ("velocityTo", velocityTo_.load(), nullptr);
    state.setProperty ("velocityAmount", velocityAmount_.load(), nullptr);
    state.setProperty ("velocitySoft", velocitySoft_.load(), nullptr);
    state.setProperty ("mpe", mpe_.load(), nullptr);
    state.setProperty ("pressureTo", pressureTo_.load(), nullptr);
    presets_.writeTo (state);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void Jane60Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts_.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram_ = static_cast<int> (tree.getProperty ("program", 0));
            demoTrim_.store (static_cast<bool> (tree.getProperty ("demoTrim", true)));
            voiceSpread_.store (static_cast<double> (tree.getProperty ("voiceSpread", 1.0)));
            vcfDriveDb_.store (static_cast<double> (tree.getProperty ("vcfDriveDb", 0.0)));
            chorusNoiseDb_.store (static_cast<double> (tree.getProperty ("chorusNoiseDb", 0.0)));
            velocityTo_.store (static_cast<int> (tree.getProperty ("velocityTo", 0)));
            velocityAmount_.store (static_cast<double> (tree.getProperty ("velocityAmount", 1.0)));
            velocitySoft_.store (static_cast<bool> (tree.getProperty ("velocitySoft", false)));
            mpe_.store (static_cast<bool> (tree.getProperty ("mpe", false)));
            pressureTo_.store (static_cast<int> (tree.getProperty ("pressureTo", 1)));
            apvts_.replaceState (tree);
            presets_.readFrom (tree);
        }
    }
}

} // namespace jane60

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new jane60::Jane60Processor();
}

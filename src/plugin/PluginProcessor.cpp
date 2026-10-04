// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginProcessor.h"
#include "Parameters.h"

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
      apvts_ (*this, nullptr, "JANE60", params::createLayout())
{
    events_.reserve (1024);
}

void Jane60Processor::prepareToPlay (double sampleRate, int)
{
    synth_.setPanel (params::readPanel (apvts_));
    synth_.prepare (calibration_, sampleRate, 440.0);
}

bool Jane60Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void Jane60Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    events_.clear();
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        MidiEvent e;
        e.sampleOffset = meta.samplePosition;
        if (m.isNoteOn())
        {
            e.type = MidiEvent::Type::noteOn;
            e.note = m.getNoteNumber();
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
            // Sustain pedal maps to the hardware HOLD jack in phase 2; ignored for now.
            continue;
        }
        else
            continue;
        if (events_.size() < events_.capacity())
            events_.push_back (e);
    }

    synth_.setPanel (params::readPanel (apvts_));

    const int n = buffer.getNumSamples();
    float* left = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    synth_.render (left, right, n, events_);
}

juce::AudioProcessorEditor* Jane60Processor::createEditor()
{
    // Phase 1: the host's generic parameter view. The panel UI arrives in phase 3.
    return new juce::GenericAudioProcessorEditor (*this);
}

void Jane60Processor::setCurrentProgram (int index)
{
    if (index < 0 || index >= static_cast<int> (factory_.size()))
        return;
    currentProgram_ = index;
    params::writePanel (apvts_, factory_[static_cast<std::size_t> (index)].panel);
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
            apvts_.replaceState (tree);
        }
    }
}

} // namespace jane60

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new jane60::Jane60Processor();
}

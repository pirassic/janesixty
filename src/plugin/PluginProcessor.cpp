// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace jane60
{

namespace
{
constexpr auto kParamMasterVolume = "masterVolume";
}

Calibration Jane60Processor::loadEmbeddedCalibration()
{
    const std::string_view text (BinaryData::juno60_json,
                                 static_cast<std::size_t> (BinaryData::juno60_jsonSize));
    return Calibration::fromJson (text);
}

juce::AudioProcessorValueTreeState::ParameterLayout Jane60Processor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Phase 0 exposes one parameter so hosts, automation and state round-trips can be validated.
    // Parameter IDs are frozen once released; this one stays.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kParamMasterVolume, 1 },
        "Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.8f));

    return layout;
}

Jane60Processor::Jane60Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      calibration_ (loadEmbeddedCalibration()),
      apvts_ (*this, nullptr, "JANE60", createParameterLayout())
{
}

void Jane60Processor::prepareToPlay (double, int) {}

bool Jane60Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void Jane60Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midi);
    buffer.clear();
}

juce::AudioProcessorEditor* Jane60Processor::createEditor()
{
    return new Jane60Editor (*this);
}

void Jane60Processor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts_.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void Jane60Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts_.state.getType()))
            apvts_.replaceState (juce::ValueTree::fromXml (*xml));
}

} // namespace jane60

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new jane60::Jane60Processor();
}

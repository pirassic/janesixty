// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginEditor.h"

namespace jane60
{

Jane60Editor::Jane60Editor (Jane60Processor& p)
    : AudioProcessorEditor (&p),
      processor_ (p),
      volumeAttachment_ (p.state(), "masterVolume", volume_)
{
    volume_.setSliderStyle (juce::Slider::LinearVertical);
    volume_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    volume_.setTitle ("Volume");
    addAndMakeVisible (volume_);

    const auto& cal = processor_.calibration();
    info_.setText (juce::String ("Jane-Sixty, phase 0. Calibration schema ")
                       + juce::String (cal.schema)
                       + ", master clock " + juce::String (cal.clock.masterClockHz, 0) + " Hz, "
                       + juce::String (static_cast<int> (cal.sources.size())) + " sourced constants, "
                       + juce::String (static_cast<int> (cal.assumedKeys().size())) + " assumed.",
                   juce::dontSendNotification);
    info_.setJustificationType (juce::Justification::centredTop);
    addAndMakeVisible (info_);

    setResizable (true, true);
    setResizeLimits (320, 240, 1600, 1200);
    setSize (480, 320);
}

void Jane60Editor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a1a));
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (22.0f));
    g.drawText ("Jane-Sixty", getLocalBounds().removeFromTop (48), juce::Justification::centred);
}

void Jane60Editor::resized()
{
    auto area = getLocalBounds().reduced (16);
    area.removeFromTop (40);
    info_.setBounds (area.removeFromTop (48));
    volume_.setBounds (area.withSizeKeepingCentre (80, juce::jmin (area.getHeight(), 200)));
}

} // namespace jane60

// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginEditor.h"

namespace jane60
{

Jane60Editor::Jane60Editor (Jane60Processor& p)
    : AudioProcessorEditor (&p),
      processor_ (p),
      generic_ (p),
      keyboard_ (p.keyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    addAndMakeVisible (generic_);
    keyboard_.setAvailableRange (36, 96);
    keyboard_.setOctaveForMiddleC (4);
    keyboard_.setKeyWidth (22.0f);
    keyboard_.setWantsKeyboardFocus (true);
    addAndMakeVisible (keyboard_);

    hint_.setText ("Click the keyboard, then play A S D F G H J K (W E T Y U for sharps); Z / X shift octaves.",
                   juce::dontSendNotification);
    hint_.setJustificationType (juce::Justification::centred);
    hint_.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (hint_);

    setResizable (true, true);
    setResizeLimits (520, 480, 1600, 1400);
    setSize (720, 760);
}

void Jane60Editor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff202020));
}

void Jane60Editor::resized()
{
    auto area = getLocalBounds();
    keyboard_.setBounds (area.removeFromBottom (90).reduced (8, 4));
    hint_.setBounds (area.removeFromBottom (20));
    generic_.setBounds (area);
}

} // namespace jane60

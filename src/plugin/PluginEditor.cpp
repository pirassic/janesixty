// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginEditor.h"
#include "ui/PanelLayout.h"

namespace jane60
{

using namespace ui::layout;

Jane60Editor::Jane60Editor (Jane60Processor& p)
    : AudioProcessorEditor (&p),
      processor_ (p),
      panel_ (p),
      keyboard_ (p.keyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard),
      demo_ (p.keyboardState())
{
    setWantsKeyboardFocus (true);
    content_.setSize (refWidth, refHeight);
    addAndMakeVisible (content_);

    panel_.setBounds (0, 0, refWidth, stripY);
    content_.addAndMakeVisible (panel_);

    keyboard_.setAvailableRange (36, 96);              // C2..C7, 61 keys
    keyboard_.setOctaveForMiddleC (4);
    keyboard_.setKeyWidth (static_cast<float> (keysW) / 36.0f);
    keyboard_.setScrollButtonsVisible (false);
    keyboard_.setBounds (keysX, keysY, keysW, keysH);
    keyboard_.setWantsKeyboardFocus (true);
    keyboard_.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xfff3f1ea));
    keyboard_.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff141414));
    keyboard_.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff6a6a6a));
    keyboard_.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour (0x66ff9f40));
    keyboard_.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour (0x22ffffff));
    content_.addAndMakeVisible (keyboard_);

    // Modern strip
    for (auto* b : { &prev_, &next_, &ab_, &copy_, &save_, &undo_ })
        content_.addAndMakeVisible (b);
    content_.addAndMakeVisible (presetBox_);
    content_.addAndMakeVisible (edited_);
    content_.addAndMakeVisible (hint_);
    edited_.setJustificationType (juce::Justification::centredLeft);
    edited_.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    edited_.setColour (juce::Label::textColourId, juce::Colour (0xffff9f40));
    hint_.setText ("Keys: A S D F G H J K play, W E T Y U sharps, Z / X octave. 1 to 7 chords, Space loops a progression. Shift + bank 1 / 2 = bank 6 / 7. See docs/user/keyboard-shortcuts.md", juce::dontSendNotification);
    hint_.setJustificationType (juce::Justification::centredRight);
    hint_.setFont (juce::FontOptions (14.0f));
    hint_.setColour (juce::Label::textColourId, juce::Colour (0xff9a9a9a));

    prev_.onClick = [this] { processor_.presets().loadPrevious(); refreshPresetList(); };
    next_.onClick = [this] { processor_.presets().loadNext(); refreshPresetList(); };
    ab_.onClick = [this] { processor_.presets().toggleAB(); refreshPresetList(); };
    copy_.onClick = [this] { processor_.presets().copyActiveToOther(); };
    save_.onClick = [this] { savePreset(); };
    undo_.onClick = [this] { processor_.undoManager().undo(); };
    presetBox_.onChange = [this]
    {
        const int idx = presetBox_.getSelectedId() - 1;
        if (idx >= 0 && idx != processor_.presets().currentIndex())
        {
            processor_.presets().load (idx);
            refreshPresetList();
        }
    };

    {
        auto strip = juce::Rectangle<int> (panelX, stripY, panelW, stripH).reduced (0, 4);
        prev_.setBounds (strip.removeFromLeft (40));
        next_.setBounds (strip.removeFromLeft (40));
        strip.removeFromLeft (8);
        undo_.setBounds (strip.removeFromRight (72));
        strip.removeFromRight (4);
        save_.setBounds (strip.removeFromRight (72));
        strip.removeFromRight (4);
        copy_.setBounds (strip.removeFromRight (72));
        strip.removeFromRight (4);
        ab_.setBounds (strip.removeFromRight (52));
        strip.removeFromRight (8);
        hint_.setBounds (strip.removeFromRight (900));
        edited_.setBounds (strip.removeFromRight (80));
        presetBox_.setBounds (strip);
    }

    refreshPresetList();
    startTimerHz (4);

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio (static_cast<double> (refWidth) / static_cast<double> (refHeight));
    setResizeLimits (1130, 425, refWidth * 2, refHeight * 2);
    // Open as wide as the display comfortably allows (the panel is a 1 m wide strip).
    int w = 1500;
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        w = juce::jlimit (1130, 1800, static_cast<int> (d->userBounds.getWidth()) - 80);
    setSize (w, static_cast<int> (std::lround (w * static_cast<double> (refHeight) / refWidth)));
}

Jane60Editor::~Jane60Editor() = default;

void Jane60Editor::refreshPresetList()
{
    auto& pm = processor_.presets();
    presetBox_.clear (juce::dontSendNotification);
    int id = 1;
    juce::String lastBank;
    for (const auto& e : pm.entries())
    {
        if (e.bank != lastBank)
        {
            presetBox_.addSectionHeading (e.bank);
            lastBank = e.bank;
        }
        presetBox_.addItem (e.name, id++);
    }
    presetBox_.setSelectedId (pm.currentIndex() + 1, juce::dontSendNotification);
    lastShownIndex_ = pm.currentIndex();
    ab_.setButtonText (pm.isSlotB() ? "B" : "A");
}

void Jane60Editor::timerCallback()
{
    auto& pm = processor_.presets();
    if (pm.currentIndex() != lastShownIndex_)
        refreshPresetList();
    edited_.setText (pm.isEdited() ? "edited" : "", juce::dontSendNotification);
}

void Jane60Editor::savePreset()
{
    auto* w = new juce::AlertWindow ("Save preset", "Name for the user preset:", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", processor_.presets().currentName(), "Name");
    w->addTextEditor ("bank", "User", "Bank");
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int result)
    {
        if (result == 1)
        {
            const auto r = processor_.presets().saveAs (w->getTextEditorContents ("name"), w->getTextEditorContents ("bank"), {});
            if (r.failed())
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Save failed", r.getErrorMessage());
            refreshPresetList();
        }
    }), true);
}

bool Jane60Editor::keyPressed (const juce::KeyPress& key)
{
    return demo_.handleKey (key);
}

void Jane60Editor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff101010));
}

void Jane60Editor::resized()
{
    const float scale = static_cast<float> (getWidth()) / static_cast<float> (refWidth);
    content_.setTransform (juce::AffineTransform::scale (scale));
}

} // namespace jane60

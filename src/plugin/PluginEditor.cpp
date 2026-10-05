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

    // Preset bar
    for (auto* b : { &prev_, &next_, &ab_, &copy_, &save_, &undo_ })
        addAndMakeVisible (b);
    addAndMakeVisible (presetBox_);
    addAndMakeVisible (edited_);
    edited_.setJustificationType (juce::Justification::centredLeft);
    edited_.setFont (juce::FontOptions (12.0f));

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
    refreshPresetList();
    startTimerHz (4);

    setResizable (true, true);
    setResizeLimits (560, 520, 1600, 1400);
    setSize (760, 800);
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
            const auto name = w->getTextEditorContents ("name");
            const auto bank = w->getTextEditorContents ("bank");
            const auto r = processor_.presets().saveAs (name, bank, {});
            if (r.failed())
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Save failed", r.getErrorMessage());
            refreshPresetList();
        }
    }), true);
}

void Jane60Editor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff202020));
}

void Jane60Editor::resized()
{
    auto area = getLocalBounds();
    auto bar = area.removeFromTop (34).reduced (8, 4);
    prev_.setBounds (bar.removeFromLeft (30));
    next_.setBounds (bar.removeFromLeft (30));
    bar.removeFromLeft (6);
    undo_.setBounds (bar.removeFromRight (56));
    bar.removeFromRight (4);
    save_.setBounds (bar.removeFromRight (56));
    bar.removeFromRight (4);
    copy_.setBounds (bar.removeFromRight (56));
    bar.removeFromRight (4);
    ab_.setBounds (bar.removeFromRight (44));
    bar.removeFromRight (6);
    edited_.setBounds (bar.removeFromRight (60));
    presetBox_.setBounds (bar);

    keyboard_.setBounds (area.removeFromBottom (90).reduced (8, 4));
    hint_.setBounds (area.removeFromBottom (20));
    generic_.setBounds (area);
}

} // namespace jane60

// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginEditor.h"
#include "ui/PanelLayout.h"

#include <cmath>

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
    for (auto* b : { &prev_, &next_, &ab_, &copy_, &save_, &undo_, &settings_ })
        content_.addAndMakeVisible (b);
    content_.addAndMakeVisible (presetBox_);
    content_.addAndMakeVisible (edited_);
    content_.addAndMakeVisible (hint_);
    content_.addAndMakeVisible (extras_);
    extras_.setJustificationType (juce::Justification::centred);
    extras_.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    extras_.setColour (juce::Label::textColourId, juce::Colour (0xff7fc8ff));
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
    settings_.onClick = [this] { showSettingsMenu (&settings_, nullptr); };
    // In the Standalone the settings live in the window's title bar (parentHierarchyChanged).
    settings_.setVisible (processor_.wrapperType != juce::AudioProcessor::wrapperType_Standalone);
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
        if (settings_.isVisible())
        {
            settings_.setBounds (strip.removeFromRight (90));
            strip.removeFromRight (4);
        }
        undo_.setBounds (strip.removeFromRight (72));
        strip.removeFromRight (4);
        save_.setBounds (strip.removeFromRight (72));
        strip.removeFromRight (4);
        copy_.setBounds (strip.removeFromRight (72));
        strip.removeFromRight (4);
        ab_.setBounds (strip.removeFromRight (52));
        strip.removeFromRight (8);
        hint_.setBounds (strip.removeFromRight (740));
        extras_.setBounds (strip.removeFromRight (70));
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
    // Badge: the extras are not part of the patch, so say when they are on.
    juce::String badge;
    if (processor_.velocity() != 0) badge << "VEL";
    if (processor_.mpe()) badge << (badge.isEmpty() ? "" : " ") << "MPE";
    extras_.setText (badge, juce::dontSendNotification);
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

void Jane60Editor::showSettingsMenu (juce::Component* target, juce::StandaloneFilterWindow* window)
{
    // Unit settings are saved with the plugin state (the project, or the Standalone's state), never in presets.
    juce::PopupMenu m;
    m.addSectionHeader ("Unit");
    m.addItem (100, "Match the factory demo unit (VCF trim one octave above the Service Notes)", true, processor_.demoTrim());
    auto is = [] (double a, double b) { return std::abs (a - b) < 1e-9; };
    {
        const double s = processor_.voiceSpread();
        juce::PopupMenu sub;
        sub.addItem (110, "Off: six identical voices", true, is (s, 0.0));
        sub.addItem (111, "Service Notes tolerances", true, is (s, 1.0));
        sub.addItem (112, "Twice the tolerances (worn unit)", true, is (s, 2.0));
        m.addSubMenu ("Voice spread", sub);
    }
    {
        const double d = processor_.vcfDriveDb();
        juce::PopupMenu sub;
        sub.addItem (120, "-6 dB: cleaner filter", true, is (d, -6.0));
        sub.addItem (121, "As calibrated (schematic reading)", true, is (d, 0.0));
        sub.addItem (122, "+6 dB: more filter growl", true, is (d, 6.0));
        m.addSubMenu ("Filter drive", sub);
    }
    {
        const double n = processor_.chorusNoiseDb();
        juce::PopupMenu sub;
        sub.addItem (130, "Off", true, n <= -90.0);
        sub.addItem (131, "As calibrated", true, is (n, 0.0));
        sub.addItem (132, "Aged BBDs (+10 dB)", true, is (n, 10.0));
        m.addSubMenu ("Chorus noise", sub);
    }
    m.addSeparator();
    m.addSectionHeader ("Extras (the hardware has none; off is bit-identical)");
    {
        const int v = processor_.velocity();
        juce::PopupMenu sub;
        sub.addItem (140, "Off (every note at full level, as the hardware)", true, v == 0);
        sub.addItem (141, "To VCA level", true, v == 1);
        sub.addItem (142, "To VCF ENV depth", true, v == 2);
        sub.addItem (143, "To both", true, v == 3);
        sub.addSeparator();
        sub.addItem (144, "Amount 50 %", true, is (processor_.velocityAmount(), 0.5));
        sub.addItem (145, "Amount 100 %", true, is (processor_.velocityAmount(), 1.0));
        sub.addItem (146, "Soft curve", true, processor_.velocitySoft());
        m.addSubMenu ("Velocity", sub);
    }
    {
        const int pr = processor_.pressure();
        juce::PopupMenu sub;
        sub.addItem (150, "MPE on (lower zone: per-note bend 48 semitones, pressure)", true, processor_.mpe());
        sub.addSeparator();
        sub.addItem (151, "Pressure off", true, pr == 0);
        sub.addItem (152, "Pressure to VCA level", true, pr == 1);
        sub.addItem (153, "Pressure to VCF ENV depth", true, pr == 2);
        sub.addItem (154, "Pressure to both", true, pr == 3);
        m.addSubMenu ("MPE", sub);
    }
    if (window != nullptr)
    {
        m.addSeparator();
        m.addItem (1, "Audio/MIDI settings...");
        m.addSeparator();
        m.addItem (2, "Save current state...");
        m.addItem (3, "Load a saved state...");
        m.addSeparator();
        m.addItem (4, "Reset to default state");
    }
    juce::Component::SafePointer<Jane60Editor> self (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target), [self, window] (int result)
    {
        if (self == nullptr || result == 0)
            return;
        auto& p = self->processor_;
        switch (result)
        {
            case 100: p.setDemoTrim (! p.demoTrim()); return;
            case 110: p.setVoiceSpread (0.0); return;
            case 111: p.setVoiceSpread (1.0); return;
            case 112: p.setVoiceSpread (2.0); return;
            case 120: p.setVcfDriveDb (-6.0); return;
            case 121: p.setVcfDriveDb (0.0); return;
            case 122: p.setVcfDriveDb (6.0); return;
            case 130: p.setChorusNoiseDb (Jane60Processor::kChorusNoiseOff); return;
            case 131: p.setChorusNoiseDb (0.0); return;
            case 132: p.setChorusNoiseDb (10.0); return;
            case 140: case 141: case 142: case 143: p.setVelocity (result - 140); return;
            case 144: p.setVelocityAmount (0.5); return;
            case 145: p.setVelocityAmount (1.0); return;
            case 146: p.setVelocitySoft (! p.velocitySoft()); return;
            case 150: p.setMpe (! p.mpe()); return;
            case 151: case 152: case 153: case 154: p.setPressure (result - 151); return;
            default: break;
        }
        if (window != nullptr)
            window->handleMenuResult (result); // 4 deletes and re-creates the plugin, this editor included
    });
}

void Jane60Editor::parentHierarchyChanged()
{
    if (standaloneHooked_ || processor_.wrapperType != juce::AudioProcessor::wrapperType_Standalone)
        return;
    auto* window = findParentComponentOfClass<juce::StandaloneFilterWindow>();
    if (window == nullptr)
        return;
    standaloneHooked_ = true;
    // The window's own "Options" button (audio settings, state) is private to JUCE; hide it and put
    // the one Settings button in its place. The editor owns the button, so it leaves with the editor.
    for (int i = 0; i < window->getNumChildComponents(); ++i)
        if (auto* b = dynamic_cast<juce::TextButton*> (window->getChildComponent (i)))
            b->setVisible (false);
    windowSettings_.setBounds (8, 6, 76, window->getTitleBarHeight() - 8);
    windowSettings_.setTriggeredOnMouseDown (true);
    windowSettings_.onClick = [this, window] { showSettingsMenu (&windowSettings_, window); };
    window->addAndMakeVisible (windowSettings_);
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

// SPDX-License-Identifier: GPL-3.0-or-later

#include "PanelComponent.h"
#include "PanelLayout.h"
#include "plugin/Parameters.h"

namespace jane60::ui
{

using namespace layout;

PanelComponent::PanelComponent (Jane60Processor& p)
    : processor_ (p), state_ (p.state())
{
    setSize (refWidth, stripY);
    buildMainPanel();
    buildMemory();
    buildBenderPanel();
    updateDisplay();
    startTimerHz (8);
}

PanelComponent::~PanelComponent() = default;

// ---------------------------------------------------------------------------
void PanelComponent::addSlider (const char* paramId, int centreX, const juce::String& legend, PanelSlider::Scale scale)
{
    auto s = std::make_unique<PanelSlider> (scale);
    s->setLegend (legend);
    s->setBounds (slider (centreX));
    s->setTitle (legend);
    addAndMakeVisible (*s);
    sliderAttachments_.push_back (std::make_unique<Apvts::SliderAttachment> (state_, paramId, *s));
    sliders_.push_back (std::move (s));
}

void PanelComponent::addSwitch (const char* paramId, int centreX, int topY, juce::StringArray legends, const juce::String& title)
{
    auto sw = std::make_unique<SlideSwitch> (legends);
    const int n = legends.size();
    sw->setBounds (vswitch (centreX, topY, n));
    sw->setTitle (title.isNotEmpty() ? title : paramId);
    addAndMakeVisible (*sw);
    auto* raw = sw.get();
    auto* param = state_.getParameter (paramId);
    auto att = std::make_unique<juce::ParameterAttachment> (*param, [raw, n] (float v)
    {
        raw->setPosition (juce::jlimit (0, n - 1, static_cast<int> (std::lround (v))), juce::dontSendNotification);
    }, state_.undoManager);
    auto* attRaw = att.get();
    sw->onChange = [attRaw] (int pos) { attRaw->setValueAsCompleteGesture (static_cast<float> (pos)); };
    att->sendInitialUpdate();
    if (title.isNotEmpty())
    {
        auto l = std::make_unique<juce::Label> ();
        l->setText (title, juce::dontSendNotification);
        l->setFont (juce::FontOptions (9.0f, juce::Font::bold));
        l->setColour (juce::Label::textColourId, colours::legend);
        l->setJustificationType (juce::Justification::centred);
        l->setBounds (centreX - 30, topY - 16, 60, 14);
        addAndMakeVisible (*l);
        labels_.push_back (std::move (l));
    }
    switchAttachments_.push_back (std::move (att));
    switches_.push_back (std::move (sw));
}

LedButton* PanelComponent::addToggle (const char* paramId, int centreX, int topY, const juce::String& legend, juce::Colour cap)
{
    auto b = std::make_unique<LedButton> (legend, cap, true, false);
    b->setBounds (button (centreX, topY));
    b->setTitle (legend);
    addAndMakeVisible (*b);
    auto* raw = b.get();
    if (paramId != nullptr)
        buttonAttachments_.push_back (std::make_unique<Apvts::ButtonAttachment> (state_, paramId, *b));
    auto l = std::make_unique<juce::Label> ();
    l->setText (legend, juce::dontSendNotification);
    l->setFont (juce::FontOptions (8.0f, juce::Font::bold));
    l->setColour (juce::Label::textColourId, colours::legend);
    l->setJustificationType (juce::Justification::centred);
    l->setBounds (centreX - 36, topY - 14, 72, 12);
    addAndMakeVisible (*l);
    labels_.push_back (std::move (l));
    buttons_.push_back (std::move (b));
    return raw;
}

// ---------------------------------------------------------------------------
void PanelComponent::buildMainPanel()
{
    const int sw = bodyY + 60; // switch top

    // POWER (decorative in software) + KEY TRANSPOSE + HOLD
    power_ = std::make_unique<SlideSwitch> (juce::StringArray { "OFF", "ON" });
    power_->setBounds (vswitch (panelX + 30, bodyY + 90, 2));
    power_->setPosition (1, juce::dontSendNotification);
    power_->setInterceptsMouseClicks (false, false);
    addAndMakeVisible (*power_);

    // KEY TRANSPOSE is a choice parameter; the panel button cycles C..B..C+ on click,
    // and the hardware gesture (hold + play a key) comes with the keyboard wiring.
    {
        auto b = std::make_unique<LedButton> ("KEY TRANSPOSE", colours::buttonCream, true, true);
        b->setBounds (button (panelX + 110, waveBtnY));
        auto* raw = b.get();
        b->onClick = [this, raw]
        {
            auto* param = state_.getParameter (params::keyTranspose);
            const int cur = static_cast<int> (std::lround (param->convertFrom0to1 (param->getValue())));
            const int next = cur == 0 ? 7 : 0; // tap: toggle between C and G (fifth) as a quick demo; the keyboard gesture sets any key
            param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (next)));
            raw->setLedOn (next != 0);
        };
        addAndMakeVisible (*b);
        auto l = std::make_unique<juce::Label> ();
        l->setText ("KEY\nTRANSPOSE", juce::dontSendNotification);
        l->setFont (juce::FontOptions (8.0f, juce::Font::bold));
        l->setColour (juce::Label::textColourId, colours::legend);
        l->setJustificationType (juce::Justification::centred);
        l->setBounds (panelX + 110 - 36, waveBtnY - 26, 72, 24);
        addAndMakeVisible (*l);
        labels_.push_back (std::move (l));
        buttons_.push_back (std::move (b));
    }
    addToggle (params::hold, panelX + 160, waveBtnY, "HOLD", colours::buttonYellow);

    // ARPEGGIO
    addToggle (params::arpOn, panelX + 225, waveBtnY, "ON/OFF", colours::buttonOrange);
    addSwitch (params::arpMode, panelX + 268, sw, { "DOWN", "UP & DOWN", "UP" }, "MODE");
    addSwitch (params::arpRange, panelX + 312, sw, { "1", "2", "3" }, "RANGE");
    addSlider (params::arpRate, panelX + 352, "RATE");

    // LFO
    addSlider (params::lfoRate, panelX + 397, "RATE");
    addSlider (params::lfoDelay, panelX + 432, "DELAY TIME");
    addSwitch (params::lfoTrig, panelX + 455, sw, { "AUTO", "MAN" }, "TRIG MODE");

    // DCO
    addSlider (params::dcoLfo, panelX + 485, "LFO");
    addSlider (params::dcoPwm, panelX + 520, "PWM");
    addSwitch (params::pwmMode, panelX + 548, sw, { "LFO", "MANUAL", "ENV" }, "PWM MODE");
    addToggle (params::pulse, panelX + 600, waveBtnY, juce::String::fromUTF8 ("⊓"), colours::buttonWhite);
    addToggle (params::saw, panelX + 640, waveBtnY, juce::String::fromUTF8 ("╱"), colours::buttonYellow);
    addToggle (params::sub, panelX + 680, waveBtnY, "SUB", colours::buttonOrange);
    addSlider (params::subLevel, panelX + 722, "SUB OSC");
    addSlider (params::noise, panelX + 757, "NOISE");

    // HPF
    addSlider (params::hpf, panelX + 802, "FREQ", PanelSlider::Scale::hpfDetents);

    // VCF
    addSlider (params::vcfFreq, panelX + 852, "FREQ");
    addSlider (params::vcfRes, panelX + 887, "RES");
    addSwitch (params::vcfPolarity, panelX + 922, sw, { "NORMAL", "INV" }, "ENV POL");
    addSlider (params::vcfEnv, panelX + 962, "ENV");
    addSlider (params::vcfLfo, panelX + 997, "LFO");
    addSlider (params::vcfKybd, panelX + 1032, "KYBD");

    // VCA
    addSwitch (params::vcaMode, panelX + 1058, sw, { "ENV", "GATE" }, "");
    addSlider (params::vcaLevel, panelX + 1098, "LEVEL", PanelSlider::Scale::minusFiveToFive);

    // ENV
    addSlider (params::attack, panelX + 1145, "A");
    addSlider (params::decay, panelX + 1180, "D");
    addSlider (params::sustain, panelX + 1215, "S");
    addSlider (params::release, panelX + 1250, "R");

    // CHORUS: three buttons on one choice parameter
    {
        auto mk = [this] (const juce::String& legend, int cx, juce::Colour cap) -> LedButton*
        {
            auto b = std::make_unique<LedButton> (legend, cap, true, true);
            b->setBounds (button (cx, waveBtnY));
            b->setTitle ("Chorus " + legend);
            addAndMakeVisible (*b);
            auto l = std::make_unique<juce::Label> ();
            l->setText (legend, juce::dontSendNotification);
            l->setFont (juce::FontOptions (8.0f, juce::Font::bold));
            l->setColour (juce::Label::textColourId, colours::legend);
            l->setJustificationType (juce::Justification::centred);
            l->setBounds (cx - 20, waveBtnY - 14, 40, 12);
            addAndMakeVisible (*l);
            labels_.push_back (std::move (l));
            auto* raw = b.get();
            buttons_.push_back (std::move (b));
            return raw;
        };
        chorusOff_ = mk ("OFF", panelX + 1292, colours::buttonWhite);
        chorusI_ = mk ("I", panelX + 1327, colours::buttonYellow);
        chorusII_ = mk ("II", panelX + 1362, colours::buttonOrange);
        chorusOff_->onClick = [this] { setChorus (0); };
        chorusI_->onClick = [this] { setChorus (chorusValue_ == 1 ? 0 : chorusValue_ == 2 ? 3 : chorusValue_ == 3 ? 2 : 1); };
        chorusII_->onClick = [this] { setChorus (chorusValue_ == 2 ? 0 : chorusValue_ == 1 ? 3 : chorusValue_ == 3 ? 1 : 2); };
        auto* param = state_.getParameter (params::chorus);
        chorusAttachment_ = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
        {
            chorusValue_ = juce::jlimit (0, 3, static_cast<int> (std::lround (v)));
            syncChorusButtons();
        }, state_.undoManager);
        chorusAttachment_->sendInitialUpdate();
    }
}

void PanelComponent::setChorus (int value)
{
    chorusValue_ = value;
    chorusAttachment_->setValueAsCompleteGesture (static_cast<float> (value));
    syncChorusButtons();
}

void PanelComponent::syncChorusButtons()
{
    if (chorusOff_ == nullptr) return;
    chorusOff_->setLedOn (chorusValue_ == 0);
    chorusI_->setLedOn (chorusValue_ == 1 || chorusValue_ == 3);
    chorusII_->setLedOn (chorusValue_ == 2 || chorusValue_ == 3);
}

// ---------------------------------------------------------------------------
void PanelComponent::buildMemory()
{
    const int x0 = panelX + 1380;
    display_.setBounds (x0 + 12, bodyY + 36, 76, 48);
    addAndMakeVisible (display_);

    static const char* bankLegends[] = { "1(6)", "2(7)", "3", "4", "5" };
    for (int i = 0; i < 5; ++i)
    {
        auto b = std::make_unique<LedButton> (bankLegends[i], colours::buttonCream, false, true);
        b->setBounds (juce::Rectangle<int> (x0 + 110 + i * 42, bodyY + 48, 34, 26));
        b->setTitle ("Bank " + juce::String (i + 1));
        const int bank = i + 1;
        b->onClick = [this, bank]
        {
            auto mods = juce::ModifierKeys::getCurrentModifiersRealtime();
            // Banks 6 and 7: hold bank 5 and press 1 or 2 on the hardware; here Shift + 1 / 2.
            int b2 = bank;
            if (mods.isShiftDown() && (bank == 1 || bank == 2)) b2 = bank + 5;
            if (writeArmed_) { armedBank_ = b2; return; }
            selectMemory (b2, shownPatch_);
        };
        addAndMakeVisible (*b);
        bankButtons_.push_back (std::move (b));
    }
    for (int i = 0; i < 8; ++i)
    {
        auto b = std::make_unique<LedButton> (juce::String (i + 1), colours::buttonCream, false, true);
        b->setBounds (juce::Rectangle<int> (x0 + 12 + i * 42, bodyY + 130, 34, 26));
        b->setTitle ("Patch " + juce::String (i + 1));
        const int patch = i + 1;
        b->onClick = [this, patch]
        {
            if (writeArmed_)
            {
                const int bank = armedBank_ > 0 ? armedBank_ : shownBank_;
                const juce::String name = juce::String (bank) + juce::String (patch);
                processor_.presets().saveAs (name, "Memory", {});
                writeArmed_ = false;
                armedBank_ = -1;
                selectMemory (bank, patch);
                return;
            }
            selectMemory (shownBank_, patch);
        };
        addAndMakeVisible (*b);
        patchButtons_.push_back (std::move (b));
    }

    auto mk = [this] (const juce::String& legend, int x, int y, juce::Colour cap, bool led)
    {
        auto b = std::make_unique<LedButton> (legend, cap, led, true);
        b->setBounds (juce::Rectangle<int> (x, y, 34, led ? 38 : 26));
        b->setTitle (legend);
        addAndMakeVisible (*b);
        auto l = std::make_unique<juce::Label> ();
        l->setText (legend, juce::dontSendNotification);
        l->setFont (juce::FontOptions (7.0f, juce::Font::bold));
        l->setColour (juce::Label::textColourId, colours::legend);
        l->setJustificationType (juce::Justification::centred);
        l->setBounds (x - 10, y - 13, 54, 12);
        addAndMakeVisible (*l);
        labels_.push_back (std::move (l));
        return b;
    };
    manual_ = mk ("MANUAL", x0 + 258, bodyY + 130, colours::buttonYellow, false);
    write_ = mk ("WRITE", x0 + 312, bodyY + 130, colours::buttonOrange, false);
    save_ = mk ("SAVE", x0 + 258, bodyY + 36, colours::buttonYellow, true);
    verify_ = mk ("VERIFY", x0 + 300, bodyY + 36, colours::buttonYellow, true);
    load_ = mk ("LOAD", x0 + 342, bodyY + 36, colours::buttonOrange, true);

    manual_->onClick = [this]
    {
        manualMode_ = true;
        writeArmed_ = false;
        updateDisplay();
    };
    write_->onClick = [this]
    {
        writeArmed_ = ! writeArmed_;
        armedBank_ = -1;
        updateDisplay();
    };
    save_->onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser> ("Export preset", juce::File::getSpecialLocation (juce::File::userDesktopDirectory), "*.json");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                              [this, chooser] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f != juce::File())
                processor_.presets().exportCurrent (f.withFileExtension ("json"), f.getFileNameWithoutExtension());
        });
    };
    load_->onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser> ("Import preset", juce::File::getSpecialLocation (juce::File::userDesktopDirectory), "*.json");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this, chooser] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f != juce::File())
            {
                processor_.presets().importFile (f);
                manualMode_ = true;
                updateDisplay();
            }
        });
    };
    verify_->onClick = [this]
    {
        // No tape to verify: flash the LED once as acknowledgement.
        verify_->setLedOn (true);
        juce::Timer::callAfterDelay (300, [this] { verify_->setLedOn (false); });
    };
}

void PanelComponent::selectMemory (int bank, int patch)
{
    bank = juce::jlimit (1, 7, bank);
    patch = juce::jlimit (1, 8, patch);
    auto& pm = processor_.presets();
    const juce::String slot = juce::String (bank) + juce::String (patch);
    // Prefer a user "Memory" slot written over this number; otherwise the factory patch.
    int target = -1;
    const auto& entries = pm.entries();
    for (std::size_t i = 0; i < entries.size(); ++i)
        if (entries[i].bank == "Memory" && entries[i].name == slot) target = static_cast<int> (i);
    if (target < 0)
        target = (bank - 1) * 8 + (patch - 1);
    pm.load (target);
    manualMode_ = false;
    shownBank_ = bank;
    shownPatch_ = patch;
    updateDisplay();
}

void PanelComponent::updateDisplay()
{
    if (writeArmed_)
        display_.setText ("__", false);
    else if (manualMode_)
        display_.setText ("--", false);
    else
        display_.setText (juce::String (shownBank_) + juce::String (shownPatch_), processor_.presets().isEdited());
}

// ---------------------------------------------------------------------------
void PanelComponent::buildBenderPanel()
{
    const int bx = benderX, by = benderY;

    benderDco_ = std::make_unique<PanelSlider> ();
    benderDco_->setLegend ("DCO");
    benderDco_->setBounds (bx + 20, by + 40, 44, 150);
    addAndMakeVisible (*benderDco_);
    benderDcoAtt_ = std::make_unique<Apvts::SliderAttachment> (state_, params::benderDco, *benderDco_);

    benderVcf_ = std::make_unique<PanelSlider> ();
    benderVcf_->setLegend ("VCF");
    benderVcf_->setBounds (bx + 62, by + 40, 44, 150);
    addAndMakeVisible (*benderVcf_);
    benderVcfAtt_ = std::make_unique<Apvts::SliderAttachment> (state_, params::benderVcf, *benderVcf_);

    volume_ = std::make_unique<PanelKnob> ();
    volume_->setBounds (bx + 150, by + 30, 70, 70);
    volume_->setTitle ("Volume");
    addAndMakeVisible (*volume_);
    volumeAtt_ = std::make_unique<Apvts::SliderAttachment> (state_, params::volume, *volume_);

    lfoTrig_ = std::make_unique<LedButton> ("LFO TRIG", colours::buttonCream, false, true);
    lfoTrig_->setBounds (bx + 120, by + 130, 60, 34);
    lfoTrig_->setTitle ("LFO Trig");
    lfoTrig_->onStateChange = [this] { processor_.setUiLfoTrig (lfoTrig_->isDown()); };
    addAndMakeVisible (*lfoTrig_);

    octave_ = std::make_unique<SlideSwitch> (juce::StringArray { "DOWN", "NORMAL", "UP" }, true);
    octave_->setBounds (bx + 195, by + 130, 90, 40);
    octave_->setTitle ("Octave Transpose");
    addAndMakeVisible (*octave_);
    {
        auto* param = state_.getParameter (params::octave);
        auto* raw = octave_.get();
        octaveAtt_ = std::make_unique<juce::ParameterAttachment> (*param, [raw] (float v)
        {
            raw->setPosition (juce::jlimit (0, 2, static_cast<int> (std::lround (v))), juce::dontSendNotification);
        }, state_.undoManager);
        auto* att = octaveAtt_.get();
        octave_->onChange = [att] (int pos) { att->setValueAsCompleteGesture (static_cast<float> (pos)); };
        octaveAtt_->sendInitialUpdate();
    }

    bender_ = std::make_unique<juce::Slider> (juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
    bender_->setRange (-1.0, 1.0, 0.0);
    bender_->setValue (0.0, juce::dontSendNotification);
    bender_->setBounds (bx + 90, by + 215, 130, 30);
    bender_->setTitle ("Bender");
    bender_->onValueChange = [this] { processor_.setUiBender (bender_->getValue()); };
    bender_->onDragEnd = [this] { bender_->setValue (0.0, juce::sendNotificationSync); }; // spring return
    addAndMakeVisible (*bender_);
}

// ---------------------------------------------------------------------------
void PanelComponent::timerCallback()
{
    // Edited dots and the key transpose LED follow the parameters.
    updateDisplay();
    auto* kt = state_.getRawParameterValue (params::keyTranspose);
    if (kt != nullptr && ! buttons_.empty())
        buttons_.front()->setLedOn (kt->load() > 0.5f);
}

void PanelComponent::resized() {}

void PanelComponent::paint (juce::Graphics& g)
{
    // Main panel
    juce::Rectangle<float> panel (panelX, panelY, panelW, panelH);
    g.setColour (colours::panelEdge);
    g.fillRoundedRectangle (panel.expanded (4.0f), 4.0f);
    g.setColour (colours::panel);
    g.fillRect (panel);

    // Section bands and separators
    for (const auto& s : sections)
    {
        juce::Rectangle<float> band (static_cast<float> (s.x0), static_cast<float> (bandY), static_cast<float> (s.x1 - s.x0), static_cast<float> (bandH));
        const auto c = s.band == creamBand ? colours::bandCream : s.band == blueBand ? colours::bandBlue : colours::bandRed;
        g.setColour (c);
        g.fillRect (band);
        drawLegend (g, s.title, band, juce::Justification::centred, 12.0f, s.band == creamBand ? colours::bandTextOnCream : colours::legend);
        g.setColour (colours::panelEdge);
        g.drawVerticalLine (s.x1, static_cast<float> (bandY), static_cast<float> (panelY + panelH));
    }
    // Bottom stripe (blue on the hardware under MEMORY, cream under POWER)
    g.setColour (colours::bandBlue);
    g.fillRect (juce::Rectangle<float> (panelX + 200.0f, panelY + panelH - 8.0f, panelW - 200.0f, 8.0f));
    g.setColour (colours::bandCream);
    g.fillRect (juce::Rectangle<float> (static_cast<float> (panelX), panelY + panelH - 8.0f, 200.0f, 8.0f));

    // Section legends that are not attached to a control
    drawLegend (g, "POWER", { panelX + 5.0f, bodyY + 40.0f, 60.0f, 14.0f }, juce::Justification::centred, 9.0f);
    drawLegend (g, "PATCH\nBANK NUMBER", { panelX + 1392.0f, bodyY + 4.0f, 80.0f, 28.0f }, juce::Justification::centred, 7.0f);
    drawLegend (g, "BANK", { panelX + 1490.0f, bodyY + 30.0f, 200.0f, 12.0f }, juce::Justification::centred, 8.0f);
    drawLegend (g, "PATCH NUMBER", { panelX + 1392.0f, bodyY + 112.0f, 340.0f, 12.0f }, juce::Justification::centred, 8.0f);
    drawLegend (g, "TAPE", { panelX + 1640.0f, bodyY + 4.0f, 140.0f, 12.0f }, juce::Justification::centred, 8.0f);
    drawLegend (g, "PROGRAMMABLE POLYPHONIC SYNTHESIZER", { panelX + 1200.0f, panelY + panelH + 6.0f, 560.0f, 18.0f }, juce::Justification::centredRight, 12.0f);
    drawLegend (g, "Jane-Sixty", { panelX + 1500.0f, panelY - 2.0f, 260.0f, 0.0f }, juce::Justification::centredRight, 1.0f);

    // Bender panel
    juce::Rectangle<float> bp (benderX, benderY, benderW, benderH);
    g.setColour (colours::panelEdge);
    g.fillRoundedRectangle (bp.expanded (4.0f), 4.0f);
    g.setColour (colours::panel);
    g.fillRect (bp);
    drawLegend (g, "VOLUME", { benderX + 140.0f, benderY + 12.0f, 90.0f, 14.0f }, juce::Justification::centred, 9.0f);
    drawLegend (g, "LFO TRIG", { benderX + 110.0f, benderY + 114.0f, 80.0f, 14.0f }, juce::Justification::centred, 9.0f);
    drawLegend (g, "OCTAVE\nTRANSPOSE", { benderX + 195.0f, benderY + 104.0f, 90.0f, 26.0f }, juce::Justification::centred, 8.0f);
    drawLegend (g, "BENDER", { benderX + 90.0f, benderY + 200.0f, 130.0f, 14.0f }, juce::Justification::centred, 9.0f);

    // Wooden end cheeks
    g.setColour (colours::wood);
    g.fillRect (juce::Rectangle<float> (0.0f, static_cast<float> (panelY) - 10.0f, 40.0f, static_cast<float> (stripY) - panelY + 10.0f));
    g.fillRect (juce::Rectangle<float> (static_cast<float> (refWidth) - 40.0f, static_cast<float> (panelY) - 10.0f, 40.0f, static_cast<float> (stripY) - panelY + 10.0f));
}

} // namespace jane60::ui

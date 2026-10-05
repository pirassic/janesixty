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
    buildBenderPanel();
    updateDisplay();
    startTimerHz (8);
}

PanelComponent::~PanelComponent() = default;

// ---------------------------------------------------------------------------
void PanelComponent::addLegend (const juce::String& text, juce::Rectangle<int> area, float size, int lines)
{
    legends_.push_back ({ text, area, size, lines });
}

void PanelComponent::addSlider (const char* paramId, int centreX, const juce::String& legend, PanelSlider::Scale scale)
{
    auto s = std::make_unique<PanelSlider> (scale);
    s->setBounds (slider (centreX));
    s->setTitle (legend);
    addAndMakeVisible (*s);
    sliderAttachments_.push_back (std::make_unique<Apvts::SliderAttachment> (state_, paramId, *s));
    sliders_.push_back (std::move (s));
    addLegend (legend, sliderLegend (centreX), fontLegend);
}

void PanelComponent::addSwitch (const char* paramId, int centreX, int width, juce::StringArray legends, const juce::String& title)
{
    auto sw = std::make_unique<SlideSwitch> (legends);
    const int n = legends.size();
    sw->setBounds (vswitch (centreX, width, n));
    sw->setTitle (title);
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
    addLegend (title, switchLegend (centreX), fontLegend);
    switchAttachments_.push_back (std::move (att));
    switches_.push_back (std::move (sw));
}

LedButton* PanelComponent::addToggle (const char* paramId, int centreX, const juce::String& legend, juce::Colour cap)
{
    auto b = std::make_unique<LedButton> (legend, cap, true, false);
    b->setBounds (button (centreX));
    b->setTitle (legend);
    addAndMakeVisible (*b);
    auto* raw = b.get();
    if (paramId != nullptr)
        buttonAttachments_.push_back (std::make_unique<Apvts::ButtonAttachment> (state_, paramId, *b));
    addLegend (legend, buttonLegend (centreX, btnCol + 16), fontLegend);
    buttons_.push_back (std::move (b));
    return raw;
}

// ---------------------------------------------------------------------------
void PanelComponent::buildMainPanel()
{
    // Columns are laid out left to right; each section records its x range for paint().
    int x = panelX;
    auto section = [&] (const juce::String& title, int band, auto&& body)
    {
        const int x0 = x;
        x += sectionPad;
        body();
        x += sectionPad;
        sections_.push_back ({ title, x0, x, band });
    };
    auto col = [&] (int w) { const int c = x + w / 2; x += w; return c; };
    constexpr int creamBand = 0, blueBand = 1, redBand = 2;
    constexpr int switchW = 58, narrowSwitchW = 44;

    section ("POWER", creamBand, [&]
    {
        const int c = col (60);
        power_ = std::make_unique<SlideSwitch> (juce::StringArray { "OFF", "ON" });
        power_->setBounds (vswitch (c, switchW, 2));
        power_->setPosition (1, juce::dontSendNotification);
        power_->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*power_);
        addLegend ("POWER", switchLegend (c), fontLegend);
    });

    section ("", creamBand, [&]
    {
        // KEY TRANSPOSE: a tap toggles C and G; the hardware gesture (hold + key) comes with MIDI.
        const int c = col (90);
        auto b = std::make_unique<LedButton> ("KEY TRANSPOSE", colours::buttonCream, true, true);
        b->setBounds (button (c));
        b->setTitle ("Key Transpose");
        keyTranspose_ = b.get();
        b->onClick = [this]
        {
            auto* param = state_.getParameter (params::keyTranspose);
            const int cur = static_cast<int> (std::lround (param->convertFrom0to1 (param->getValue())));
            const int next = cur == 0 ? 7 : 0;
            param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (next)));
        };
        addAndMakeVisible (*b);
        buttons_.push_back (std::move (b));
        addLegend ("KEY\nTRANSPOSE", buttonLegend (c, 90), fontLegend);
        addToggle (params::hold, col (48), "HOLD", colours::buttonYellow);
    });

    section ("ARPEGGIO", blueBand, [&]
    {
        addToggle (params::arpOn, col (48), "ON/OFF", colours::buttonOrange);
        addSwitch (params::arpMode, col (switchW), switchW, { "DOWN", "U/D", "UP" }, "MODE");
        addSwitch (params::arpRange, col (narrowSwitchW), narrowSwitchW, { "1", "2", "3" }, "RANGE");
        addSlider (params::arpRate, col (sliderCol), "RATE");
    });

    section ("LFO", redBand, [&]
    {
        addSlider (params::lfoRate, col (sliderCol), "RATE");
        addSlider (params::lfoDelay, col (sliderCol), "DELAY\nTIME");
        addSwitch (params::lfoTrig, col (switchW), switchW, { "AUTO", "MAN" }, "TRIG");
    });

    section ("DCO", redBand, [&]
    {
        addSlider (params::dcoLfo, col (sliderCol), "LFO");
        addSlider (params::dcoPwm, col (sliderCol), "PWM");
        addSwitch (params::pwmMode, col (switchW), switchW, { "LFO", "MAN", "ENV" }, "PWM\nMODE");
        addToggle (params::pulse, col (btnCol), juce::String::fromUTF8 ("\xe2\x8a\x93"), colours::buttonWhite);   // pulse glyph
        addToggle (params::saw, col (btnCol), juce::String::fromUTF8 ("\xe2\x95\xb1"), colours::buttonYellow);    // ramp glyph
        addToggle (params::sub, col (btnCol), "SUB", colours::buttonOrange);
        addSlider (params::subLevel, col (sliderCol), "SUB\nOSC");
        addSlider (params::noise, col (sliderCol), "NOISE");
    });

    section ("HPF", redBand, [&]
    {
        addSlider (params::hpf, col (sliderCol + 4), "FREQ", PanelSlider::Scale::hpfDetents);
    });

    section ("VCF", redBand, [&]
    {
        addSlider (params::vcfFreq, col (sliderCol), "FREQ");
        addSlider (params::vcfRes, col (sliderCol), "RES");
        addSwitch (params::vcfPolarity, col (switchW), switchW, { "NORM", "INV" }, "ENV\nPOL");
        addSlider (params::vcfEnv, col (sliderCol), "ENV");
        addSlider (params::vcfLfo, col (sliderCol), "LFO");
        addSlider (params::vcfKybd, col (sliderCol), "KYBD");
    });

    section ("VCA", redBand, [&]
    {
        addSwitch (params::vcaMode, col (switchW), switchW, { "ENV", "GATE" }, "MODE");
        addSlider (params::vcaLevel, col (sliderCol), "LEVEL", PanelSlider::Scale::minusFiveToFive);
    });

    section ("ENV", redBand, [&]
    {
        addSlider (params::attack, col (sliderCol), "A");
        addSlider (params::decay, col (sliderCol), "D");
        addSlider (params::sustain, col (sliderCol), "S");
        addSlider (params::release, col (sliderCol), "R");
    });

    section ("CHORUS", redBand, [&]
    {
        auto mk = [this] (const juce::String& legend, int cx, juce::Colour cap) -> LedButton*
        {
            auto b = std::make_unique<LedButton> (legend, cap, true, true);
            b->setBounds (button (cx));
            b->setTitle ("Chorus " + legend);
            addAndMakeVisible (*b);
            addLegend (legend, buttonLegend (cx, btnCol), fontLegend);
            auto* raw = b.get();
            buttons_.push_back (std::move (b));
            return raw;
        };
        chorusOff_ = mk ("OFF", col (btnCol), colours::buttonWhite);
        chorusI_ = mk ("I", col (btnCol), colours::buttonYellow);
        chorusII_ = mk ("II", col (btnCol), colours::buttonOrange);
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
    });

    // MEMORY takes the rest of the panel.
    {
        const int x0 = x, x1 = panelX + panelW;
        sections_.push_back ({ "MEMORY", x0, x1, blueBand });
        buildMemory (x0 + sectionPad, x1 - sectionPad);
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
std::unique_ptr<LedButton> PanelComponent::addMemoryButton (const juce::String& capText, int x, int y, int w, juce::Colour cap, bool led, const juce::String& title)
{
    auto b = std::make_unique<LedButton> (title, cap, led, true);
    b->setBounds (juce::Rectangle<int> (x, led ? y - btnLedH : y, w, led ? memBtnH + btnLedH : memBtnH));
    b->setCapText (capText);
    b->setTitle (title);
    addAndMakeVisible (*b);
    return b;
}

void PanelComponent::buildMemory (int x0, int x1)
{
    // Row 1: display, BANK 1..5, MANUAL, WRITE. Row 2: PATCH NUMBER 1..8, TAPE SAVE / VERIFY / LOAD.
    const int row1 = bodyY + 70, row2 = bodyY + 180;
    const int rowLegendH = 20;

    display_.setBounds (x0, row1 - 14, 80, 54);
    addAndMakeVisible (display_);
    addLegend ("BANK / PATCH", { x0 - 6, row1 - 14 - rowLegendH - 2, 92, rowLegendH }, fontLegend - 2.0f, 1);

    int bx = x0 + 80 + 14;
    addLegend ("BANK", { bx, row1 - rowLegendH - 4, 5 * memBtnCol, rowLegendH }, fontLegend - 2.0f, 1);
    for (int i = 0; i < 5; ++i)
    {
        const int bank = i + 1;
        auto b = addMemoryButton (juce::String (bank), bx + i * memBtnCol, row1, memBtnW, colours::buttonCream, false,
                                  "Bank " + juce::String (bank) + (bank <= 2 ? " (Shift-click: bank " + juce::String (bank + 5) + ")" : juce::String()));
        b->onClick = [this, bank]
        {
            auto mods = juce::ModifierKeys::getCurrentModifiersRealtime();
            // Banks 6 and 7: hold bank 5 and press 1 or 2 on the hardware; here Shift + 1 / 2.
            int b2 = bank;
            if (mods.isShiftDown() && (bank == 1 || bank == 2)) b2 = bank + 5;
            if (writeArmed_) { armedBank_ = b2; return; }
            selectMemory (b2, shownPatch_);
        };
        bankButtons_.push_back (std::move (b));
    }
    bx += 5 * memBtnCol + 16;
    const int wideW = 48;
    manual_ = addMemoryButton ("MAN", bx, row1, wideW, colours::buttonYellow, false, "Manual");
    write_ = addMemoryButton ("WRITE", bx + wideW + 6, row1, wideW, colours::buttonOrange, false, "Write");
    addLegend ("MANUAL", { bx - 8, row1 - rowLegendH - 4, wideW + 16, rowLegendH }, fontLegend - 2.0f, 1);
    addLegend ("WRITE", { bx + wideW + 6 - 8, row1 - rowLegendH - 4, wideW + 16, rowLegendH }, fontLegend - 2.0f, 1);

    int px = x0;
    addLegend ("PATCH NUMBER", { px, row2 - rowLegendH - 4, 8 * memBtnCol, rowLegendH }, fontLegend - 2.0f, 1);
    for (int i = 0; i < 8; ++i)
    {
        const int patch = i + 1;
        auto b = addMemoryButton (juce::String (patch), px + i * memBtnCol, row2, memBtnW, colours::buttonCream, false, "Patch " + juce::String (patch));
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
        patchButtons_.push_back (std::move (b));
    }
    px += 8 * memBtnCol + 16;
    const int tapeW = juce::jmax (3 * (memBtnW + 10), x1 - px);
    addLegend ("TAPE / FILE", { px, row2 - rowLegendH - 4, tapeW, rowLegendH }, fontLegend - 2.0f, 1);
    const int tapeCol = tapeW / 3, tapeBtnW = juce::jmin (memBtnW + 10, tapeCol - 4);
    save_ = addMemoryButton ("SAVE", px, row2 + btnLedH, tapeBtnW, colours::buttonYellow, true, "Save preset file");
    verify_ = addMemoryButton ("VER", px + tapeCol, row2 + btnLedH, tapeBtnW, colours::buttonYellow, true, "Verify (no tape; flashes)");
    load_ = addMemoryButton ("LOAD", px + 2 * tapeCol, row2 + btnLedH, tapeBtnW, colours::buttonOrange, true, "Load preset file");

    manual_->onClick = [this]
    {
        manualMode_ = true;
        manualIndex_ = processor_.presets().currentIndex();
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
    // The display follows whatever is loaded, from any selector: factory patch -> its number,
    // a written Memory slot -> its number, any other user preset -> "--" (no memory number).
    auto& pm = processor_.presets();
    const int idx = pm.currentIndex();
    if (manualMode_ && idx != manualIndex_)
        manualMode_ = false;

    juce::String text = "--";
    const auto& entries = pm.entries();
    if (idx >= 0 && idx < static_cast<int> (entries.size()))
    {
        const auto& e = entries[static_cast<std::size_t> (idx)];
        if (e.factoryIndex >= 0)
        {
            shownBank_ = e.factoryIndex / 8 + 1;
            shownPatch_ = e.factoryIndex % 8 + 1;
            text = juce::String (shownBank_) + juce::String (shownPatch_);
        }
        else if (e.bank == "Memory" && e.name.length() == 2 && e.name.containsOnly ("0123456789"))
        {
            shownBank_ = e.name.substring (0, 1).getIntValue();
            shownPatch_ = e.name.substring (1, 2).getIntValue();
            text = e.name;
        }
    }

    if (writeArmed_)
        display_.setText ("__", false);
    else if (manualMode_)
        display_.setText ("--", false);
    else
        display_.setText (text, pm.isEdited());
}

// ---------------------------------------------------------------------------
void PanelComponent::buildBenderPanel()
{
    const int bx = benderX, by = benderY;

    benderDco_ = std::make_unique<PanelSlider> ();
    benderDco_->setBounds (bx + 24, by + 56, sliderCol, 170);
    benderDco_->setTitle ("Bender DCO depth");
    addAndMakeVisible (*benderDco_);
    benderDcoAtt_ = std::make_unique<Apvts::SliderAttachment> (state_, params::benderDco, *benderDco_);
    addLegend ("DCO", { bx + 14, by + 20, sliderCol + 20, legendH }, fontLegend);

    benderVcf_ = std::make_unique<PanelSlider> ();
    benderVcf_->setBounds (bx + 74, by + 56, sliderCol, 170);
    benderVcf_->setTitle ("Bender VCF depth");
    addAndMakeVisible (*benderVcf_);
    benderVcfAtt_ = std::make_unique<Apvts::SliderAttachment> (state_, params::benderVcf, *benderVcf_);
    addLegend ("VCF", { bx + 64, by + 20, sliderCol + 20, legendH }, fontLegend);

    volume_ = std::make_unique<PanelKnob> ();
    volume_->setBounds (bx + 150, by + 44, 100, 100);
    volume_->setTitle ("Volume");
    addAndMakeVisible (*volume_);
    volumeAtt_ = std::make_unique<Apvts::SliderAttachment> (state_, params::volume, *volume_);
    addLegend ("VOLUME", { bx + 140, by + 12, 120, legendH }, fontLegend);

    lfoTrig_ = std::make_unique<LedButton> ("LFO TRIG", colours::buttonCream, false, true);
    lfoTrig_->setBounds (bx + 136, by + 186, 56, 36);
    lfoTrig_->setCapText ("TRIG");
    lfoTrig_->setTitle ("LFO Trig");
    lfoTrig_->onStateChange = [this] { processor_.setUiLfoTrig (lfoTrig_->isDown()); };
    addAndMakeVisible (*lfoTrig_);
    addLegend ("LFO\nTRIG", { bx + 120, by + 150, 88, legendH }, fontLegend);

    octave_ = std::make_unique<SlideSwitch> (juce::StringArray { "DOWN", "NORMAL", "UP" }, true);
    octave_->setBounds (bx + 196, by + 180, 100, 44);
    octave_->setTitle ("Octave Transpose");
    addAndMakeVisible (*octave_);
    addLegend ("OCTAVE\nTRANSPOSE", { bx + 196, by + 146, 100, legendH }, fontLegend);
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
    bender_->setBounds (bx + 70, by + 290, 160, 34);
    bender_->setTitle ("Bender");
    bender_->onValueChange = [this] { processor_.setUiBender (bender_->getValue()); };
    bender_->onDragEnd = [this] { bender_->setValue (0.0, juce::sendNotificationSync); }; // spring return
    addAndMakeVisible (*bender_);
    addLegend ("BENDER", { bx + 70, by + 258, 160, legendH }, fontLegend);
}

// ---------------------------------------------------------------------------
void PanelComponent::timerCallback()
{
    // Display, edited dots and the key transpose LED follow the parameters and the preset list.
    updateDisplay();
    auto* kt = state_.getRawParameterValue (params::keyTranspose);
    if (kt != nullptr && keyTranspose_ != nullptr)
        keyTranspose_->setLedOn (kt->load() > 0.5f);
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
    for (const auto& s : sections_)
    {
        juce::Rectangle<float> band (static_cast<float> (s.x0), static_cast<float> (bandY), static_cast<float> (s.x1 - s.x0), static_cast<float> (bandH));
        const auto c = s.band == 0 ? colours::bandCream : s.band == 1 ? colours::bandBlue : colours::bandRed;
        g.setColour (c);
        g.fillRect (band);
        drawLegend (g, s.title, band, juce::Justification::centred, fontBand, s.band == 0 ? colours::bandTextOnCream : colours::legend);
        g.setColour (colours::panelEdge);
        g.fillRect (juce::Rectangle<float> (static_cast<float> (s.x1) - 1.0f, static_cast<float> (bandY), 2.0f, static_cast<float> (panelH)));
    }
    // Bottom stripe (blue on the hardware under MEMORY, cream under POWER)
    g.setColour (colours::bandBlue);
    g.fillRect (juce::Rectangle<float> (panelX + 220.0f, panelY + panelH - 8.0f, panelW - 220.0f, 8.0f));
    g.setColour (colours::bandCream);
    g.fillRect (juce::Rectangle<float> (static_cast<float> (panelX), panelY + panelH - 8.0f, 220.0f, 8.0f));

    // Bender panel
    juce::Rectangle<float> bp (benderX, benderY, benderW, benderH);
    g.setColour (colours::panelEdge);
    g.fillRoundedRectangle (bp.expanded (4.0f), 4.0f);
    g.setColour (colours::panel);
    g.fillRect (bp);

    // Control legends (after every background fill)
    for (const auto& l : legends_)
    {
        g.setColour (colours::legend);
        g.setFont (juce::FontOptions (l.size, juce::Font::bold));
        g.drawFittedText (l.text, l.area, l.lines > 1 ? juce::Justification::centredBottom : juce::Justification::centred, l.lines);
    }

    drawLegend (g, "PROGRAMMABLE POLYPHONIC SYNTHESIZER", { panelX + panelW - 560.0f, panelY + panelH + 6.0f, 560.0f, 20.0f }, juce::Justification::centredRight, 14.0f);

    // Wooden end cheeks
    g.setColour (colours::wood);
    g.fillRect (juce::Rectangle<float> (0.0f, static_cast<float> (panelY) - 10.0f, 40.0f, static_cast<float> (stripY) - panelY + 10.0f));
    g.fillRect (juce::Rectangle<float> (static_cast<float> (refWidth) - 40.0f, static_cast<float> (panelY) - 10.0f, 40.0f, static_cast<float> (stripY) - panelY + 10.0f));
}

} // namespace jane60::ui

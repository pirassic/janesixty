// SPDX-License-Identifier: GPL-3.0-or-later
// The main panel strip and the bender panel, in reference coordinates.
#pragma once

#include <JuceHeader.h>

#include "Controls.h"
#include "plugin/PluginProcessor.h"

#include <memory>
#include <vector>

namespace jane60::ui
{

class PanelComponent final : public juce::Component,
                             private juce::Timer
{
public:
    explicit PanelComponent (Jane60Processor& p);
    ~PanelComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    using Apvts = juce::AudioProcessorValueTreeState;

    struct Section { juce::String title; int x0, x1, band; };
    struct Legend { juce::String text; juce::Rectangle<int> area; float size; int lines; };

    void timerCallback() override;
    void buildMainPanel();
    void buildBenderPanel();
    void buildMemory (int x0, int x1);
    void addLegend (const juce::String& text, juce::Rectangle<int> area, float size, int lines = 2);
    void addSlider (const char* paramId, int centreX, const juce::String& legend, PanelSlider::Scale scale = PanelSlider::Scale::zeroToTen);
    void addSwitch (const char* paramId, int centreX, int width, juce::StringArray legends, const juce::String& title);
    LedButton* addToggle (const char* paramId, int centreX, const juce::String& legend, juce::Colour cap);
    std::unique_ptr<LedButton> addMemoryButton (const juce::String& capText, int x, int y, int w, juce::Colour cap, bool led, const juce::String& title);
    void selectMemory (int bank, int patch);
    void updateDisplay();
    void syncChorusButtons();
    void setChorus (int value);

    Jane60Processor& processor_;
    Apvts& state_;

    std::vector<Section> sections_;
    std::vector<Legend> legends_;

    // Owned controls
    std::vector<std::unique_ptr<PanelSlider>> sliders_;
    std::vector<std::unique_ptr<Apvts::SliderAttachment>> sliderAttachments_;
    std::vector<std::unique_ptr<LedButton>> buttons_;
    std::vector<std::unique_ptr<Apvts::ButtonAttachment>> buttonAttachments_;
    std::vector<std::unique_ptr<SlideSwitch>> switches_;
    std::vector<std::unique_ptr<juce::ParameterAttachment>> switchAttachments_;
    LedButton* keyTranspose_ = nullptr;

    // Chorus: three buttons driving one choice parameter
    LedButton* chorusOff_ = nullptr; LedButton* chorusI_ = nullptr; LedButton* chorusII_ = nullptr;
    std::unique_ptr<juce::ParameterAttachment> chorusAttachment_;
    int chorusValue_ = 0;

    // Memory section
    SegmentDisplay display_;
    std::vector<std::unique_ptr<LedButton>> bankButtons_, patchButtons_;
    std::unique_ptr<LedButton> manual_, write_, save_, verify_, load_;
    bool writeArmed_ = false;
    int armedBank_ = -1;
    bool manualMode_ = false;
    int manualIndex_ = -1;       // preset index at which MANUAL was pressed
    int shownBank_ = 1, shownPatch_ = 1;

    // Bender panel
    std::unique_ptr<PanelSlider> benderDco_, benderVcf_;
    std::unique_ptr<Apvts::SliderAttachment> benderDcoAtt_, benderVcfAtt_;
    std::unique_ptr<PanelKnob> volume_;
    std::unique_ptr<Apvts::SliderAttachment> volumeAtt_;
    std::unique_ptr<LedButton> lfoTrig_;
    std::unique_ptr<SlideSwitch> octave_;
    std::unique_ptr<juce::ParameterAttachment> octaveAtt_;
    std::unique_ptr<juce::Slider> bender_;

    // Power
    std::unique_ptr<SlideSwitch> power_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanelComponent)
};

} // namespace jane60::ui

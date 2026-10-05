// SPDX-License-Identifier: GPL-3.0-or-later
// Panel controls drawn in code: slider with printed scale, latching LED button,
// slide switch, LED, seven-segment display. All sizes are in the panel's
// reference coordinate space (see PanelLayout.h); the editor scales the whole panel.
#pragma once

#include <JuceHeader.h>

namespace jane60::ui
{

namespace colours
{
inline const juce::Colour panel { 0xff1c1c1e };
inline const juce::Colour panelEdge { 0xff0d0d0e };
inline const juce::Colour legend { 0xffe8e6df };
inline const juce::Colour legendDim { 0xffb5b3ac };
inline const juce::Colour bandRed { 0xffb0372f };
inline const juce::Colour bandBlue { 0xff2f5fa8 };
inline const juce::Colour bandCream { 0xffe6e1d3 };
inline const juce::Colour bandTextOnCream { 0xff2a2a2a };
inline const juce::Colour sliderSlot { 0xff0a0a0a };
inline const juce::Colour sliderCap { 0xff262626 };
inline const juce::Colour sliderCapLine { 0xffe9e9e9 };
inline const juce::Colour buttonCream { 0xffe9dfbd };
inline const juce::Colour buttonOrange { 0xffe08a2b };
inline const juce::Colour buttonYellow { 0xffe9c54a };
inline const juce::Colour buttonWhite { 0xfff2efe6 };
inline const juce::Colour buttonGrey { 0xff8c8c8c };
inline const juce::Colour ledOff { 0xff4a1410 };
inline const juce::Colour ledOn { 0xffff3b2a };
inline const juce::Colour segOff { 0xff3a0f0c };
inline const juce::Colour segOn { 0xffff4a30 };
inline const juce::Colour wood { 0xff5a3420 };
} // namespace colours

/// A vertical panel slider with a printed scale. Scale labels are configurable
/// (0..10 by default, -5..+5 for LEVEL, 0..3 detented for HPF).
class PanelSlider final : public juce::Slider
{
public:
    enum class Scale { zeroToTen, minusFiveToFive, hpfDetents };

    explicit PanelSlider (Scale scale = Scale::zeroToTen);
    void paint (juce::Graphics& g) override;
    void setLegend (const juce::String& text) { legend_ = text; repaint(); }

private:
    Scale scale_;
    juce::String legend_;
};

/// Latching push button with an LED above it (DCO waveform, chorus, arpeggio, hold, key transpose).
class LedButton final : public juce::Button
{
public:
    LedButton (const juce::String& legend, juce::Colour cap, bool hasLed = true, bool momentary = false);
    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
    void setLedOn (bool on) { ledOn_ = on; repaint(); }
    bool isMomentary() const noexcept { return momentary_; }

private:
    juce::String legend_;
    juce::Colour cap_;
    bool hasLed_, momentary_, ledOn_ = false;
};

/// Two- or three-position slide switch (vertical by default) with legends per position.
class SlideSwitch final : public juce::Component
{
public:
    SlideSwitch (juce::StringArray positionLegends, bool horizontal = false);
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

    int getPosition() const noexcept { return position_; }
    void setPosition (int p, juce::NotificationType n = juce::sendNotification);
    std::function<void (int)> onChange;
    int numPositions() const noexcept { return legends_.size(); }

private:
    void pick (const juce::MouseEvent& e);
    juce::StringArray legends_;
    bool horizontal_;
    int position_ = 0;
};

/// Two-digit seven-segment display with two decimal points.
class SegmentDisplay final : public juce::Component
{
public:
    void paint (juce::Graphics& g) override;
    void setText (const juce::String& twoChars, bool dots);

private:
    static void drawDigit (juce::Graphics& g, juce::Rectangle<float> r, juce::juce_wchar ch, bool dot);
    juce::String text_ { "--" };
    bool dots_ = false;
};

/// Small red LED (standalone, for indicators that sit away from a button).
class Led final : public juce::Component
{
public:
    void paint (juce::Graphics& g) override;
    void setOn (bool on) { on_ = on; repaint(); }

private:
    bool on_ = false;
};

/// Rotary VOLUME knob with an index line.
class PanelKnob final : public juce::Slider
{
public:
    PanelKnob();
    void paint (juce::Graphics& g) override;
};

/// Draws legend text in the panel style.
void drawLegend (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                 juce::Justification just = juce::Justification::centred, float size = 11.0f,
                 juce::Colour colour = colours::legend);

} // namespace jane60::ui

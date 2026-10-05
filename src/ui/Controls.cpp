// SPDX-License-Identifier: GPL-3.0-or-later

#include "Controls.h"

namespace jane60::ui
{

void drawLegend (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                 juce::Justification just, float size, juce::Colour colour)
{
    g.setColour (colour);
    g.setFont (juce::FontOptions (size, juce::Font::bold));
    g.drawText (text, area, just, false);
}

// ---------------------------------------------------------------------------
PanelSlider::PanelSlider (Scale scale) : scale_ (scale)
{
    setSliderStyle (juce::Slider::LinearVertical);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    setMouseDragSensitivity (160);
    setVelocityBasedMode (false);
    setDoubleClickReturnValue (true, scale == Scale::minusFiveToFive ? 0.0 : 0.0);
    setSliderSnapsToMousePosition (false);
    if (scale == Scale::hpfDetents)
        setRange (0.0, 3.0, 1.0);
}

void PanelSlider::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float slotX = b.getCentreX();
    const float top = b.getY() + 10.0f, bottom = b.getBottom() - 10.0f;
    const float travel = bottom - top;

    // Scale ticks and labels
    const int ticks = scale_ == Scale::hpfDetents ? 3 : 10;
    for (int i = 0; i <= ticks; ++i)
    {
        const float y = bottom - travel * static_cast<float> (i) / static_cast<float> (ticks);
        const bool major = scale_ == Scale::hpfDetents || i % 5 == 0;
        g.setColour (colours::legendDim);
        const float len = major ? 7.0f : 4.0f;
        g.drawLine (slotX - 6.0f - len, y, slotX - 6.0f, y, 1.0f);
        g.drawLine (slotX + 6.0f, y, slotX + 6.0f + len, y, 1.0f);
        if (major)
        {
            juce::String label;
            if (scale_ == Scale::zeroToTen) label = juce::String (i);
            else if (scale_ == Scale::hpfDetents) label = juce::String (i);
            else label = i == 0 ? "-5" : i == 5 ? "0" : "+5";
            g.setFont (juce::FontOptions (8.0f));
            g.drawText (label, juce::Rectangle<float> (slotX - 30.0f, y - 6.0f, 16.0f, 12.0f), juce::Justification::centredRight, false);
        }
    }

    // Slot
    g.setColour (colours::sliderSlot);
    g.fillRoundedRectangle (slotX - 2.5f, top - 4.0f, 5.0f, travel + 8.0f, 2.0f);

    // Cap: black, chamfered, white index line
    const double range = getMaximum() - getMinimum();
    const float pos = range > 0.0 ? static_cast<float> ((getValue() - getMinimum()) / range) : 0.0f;
    const float capY = bottom - travel * pos;
    juce::Rectangle<float> cap (slotX - 11.0f, capY - 9.0f, 22.0f, 18.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (cap.translated (1.5f, 2.0f), 2.0f);
    g.setGradientFill (juce::ColourGradient (colours::sliderCap.brighter (0.25f), cap.getX(), cap.getY(),
                                             colours::sliderCap.darker (0.4f), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 2.0f);
    g.setColour (colours::sliderCapLine);
    g.fillRect (cap.getX() + 2.0f, capY - 0.75f, cap.getWidth() - 4.0f, 1.5f);

    if (legend_.isNotEmpty())
        drawLegend (g, legend_, juce::Rectangle<float> (b.getX() - 10.0f, b.getY() - 14.0f, b.getWidth() + 20.0f, 12.0f), juce::Justification::centred, 9.0f);
}

// ---------------------------------------------------------------------------
LedButton::LedButton (const juce::String& legend, juce::Colour cap, bool hasLed, bool momentary)
    : juce::Button (legend), legend_ (legend), cap_ (cap), hasLed_ (hasLed), momentary_ (momentary)
{
    setClickingTogglesState (! momentary);
}

void LedButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto b = getLocalBounds().toFloat();
    if (hasLed_)
    {
        auto ledArea = b.removeFromTop (12.0f);
        const bool on = momentary_ ? ledOn_ : (getToggleState() || ledOn_);
        juce::Rectangle<float> led (ledArea.getCentreX() - 3.5f, ledArea.getCentreY() - 3.5f, 7.0f, 7.0f);
        if (on)
        {
            g.setColour (colours::ledOn.withAlpha (0.35f));
            g.fillEllipse (led.expanded (3.0f));
        }
        g.setColour (on ? colours::ledOn : colours::ledOff);
        g.fillEllipse (led);
    }
    auto cap = b.reduced (1.0f);
    const bool pressed = down || (momentary_ ? false : false);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (cap.translated (1.0f, 2.0f), 3.0f);
    auto c = highlighted ? cap_.brighter (0.08f) : cap_;
    if (pressed) c = c.darker (0.15f);
    g.setGradientFill (juce::ColourGradient (c.brighter (0.15f), cap.getX(), cap.getY(), c.darker (0.2f), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (cap, 3.0f, 1.0f);
}

// ---------------------------------------------------------------------------
SlideSwitch::SlideSwitch (juce::StringArray positionLegends, bool horizontal)
    : legends_ (std::move (positionLegends)), horizontal_ (horizontal)
{
}

void SlideSwitch::setPosition (int p, juce::NotificationType n)
{
    p = juce::jlimit (0, numPositions() - 1, p);
    if (p == position_) return;
    position_ = p;
    repaint();
    if (n != juce::dontSendNotification && onChange) onChange (position_);
}

void SlideSwitch::pick (const juce::MouseEvent& e)
{
    const int n = numPositions();
    if (horizontal_)
        setPosition (juce::jlimit (0, n - 1, static_cast<int> (e.position.x / (static_cast<float> (getWidth()) / static_cast<float> (n)))));
    else
        setPosition (juce::jlimit (0, n - 1, (n - 1) - static_cast<int> (e.position.y / (static_cast<float> (getHeight()) / static_cast<float> (n)))));
}

void SlideSwitch::mouseDown (const juce::MouseEvent& e) { pick (e); }
void SlideSwitch::mouseDrag (const juce::MouseEvent& e) { pick (e); }

void SlideSwitch::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const int n = numPositions();
    if (horizontal_)
    {
        const float cellW = b.getWidth() / static_cast<float> (n);
        juce::Rectangle<float> slot (b.getX() + 4.0f, b.getCentreY() - 5.0f, b.getWidth() - 8.0f, 10.0f);
        g.setColour (colours::sliderSlot);
        g.fillRoundedRectangle (slot, 3.0f);
        const float kx = b.getX() + cellW * (static_cast<float> (position_) + 0.5f);
        g.setColour (colours::sliderCap.brighter (0.2f));
        g.fillRoundedRectangle (kx - 7.0f, b.getCentreY() - 8.0f, 14.0f, 16.0f, 2.0f);
        for (int i = 0; i < n; ++i)
            drawLegend (g, legends_[i], juce::Rectangle<float> (b.getX() + cellW * static_cast<float> (i), b.getBottom() - 12.0f, cellW, 12.0f), juce::Justification::centred, 7.0f);
    }
    else
    {
        const float cellH = b.getHeight() / static_cast<float> (n);
        juce::Rectangle<float> slot (b.getX() + 6.0f, b.getY() + 6.0f, 10.0f, b.getHeight() - 12.0f);
        g.setColour (colours::sliderSlot);
        g.fillRoundedRectangle (slot, 3.0f);
        const float ky = b.getY() + cellH * (static_cast<float> (n - 1 - position_) + 0.5f);
        g.setColour (colours::sliderCap.brighter (0.2f));
        g.fillRoundedRectangle (slot.getCentreX() - 8.0f, ky - 7.0f, 16.0f, 14.0f, 2.0f);
        for (int i = 0; i < n; ++i)
        {
            const float y = b.getY() + cellH * static_cast<float> (n - 1 - i);
            drawLegend (g, legends_[i], juce::Rectangle<float> (slot.getRight() + 3.0f, y, b.getWidth() - slot.getRight() - 2.0f, cellH), juce::Justification::centredLeft, 7.0f);
        }
    }
}

// ---------------------------------------------------------------------------
void SegmentDisplay::setText (const juce::String& twoChars, bool dots)
{
    text_ = twoChars.substring (0, 2).paddedLeft (' ', 2);
    dots_ = dots;
    repaint();
}

void SegmentDisplay::drawDigit (juce::Graphics& g, juce::Rectangle<float> r, juce::juce_wchar ch, bool dot)
{
    // Segment mask: a b c d e f g (bit 0 = a top, clockwise, bit 6 = g middle)
    int mask = 0;
    switch (ch)
    {
        case '0': mask = 0x3F; break; case '1': mask = 0x06; break; case '2': mask = 0x5B; break;
        case '3': mask = 0x4F; break; case '4': mask = 0x66; break; case '5': mask = 0x6D; break;
        case '6': mask = 0x7D; break; case '7': mask = 0x07; break; case '8': mask = 0x7F; break;
        case '9': mask = 0x6F; break; case '-': mask = 0x40; break; case '_': mask = 0x08; break;
        case 'E': mask = 0x79; break; case 'r': mask = 0x50; break; case 'P': mask = 0x73; break;
        default: mask = 0; break;
    }
    const float w = r.getWidth(), h = r.getHeight(), t = w * 0.16f;
    auto seg = [&] (int bit, juce::Rectangle<float> rect)
    {
        g.setColour ((mask >> bit) & 1 ? colours::segOn : colours::segOff);
        g.fillRoundedRectangle (rect, t * 0.4f);
    };
    const float x = r.getX(), y = r.getY();
    seg (0, { x + t, y, w - 2 * t, t });                       // a
    seg (1, { x + w - t, y + t * 0.5f, t, h * 0.5f - t });     // b
    seg (2, { x + w - t, y + h * 0.5f + t * 0.5f, t, h * 0.5f - t }); // c
    seg (3, { x + t, y + h - t, w - 2 * t, t });               // d
    seg (4, { x, y + h * 0.5f + t * 0.5f, t, h * 0.5f - t });  // e
    seg (5, { x, y + t * 0.5f, t, h * 0.5f - t });             // f
    seg (6, { x + t, y + h * 0.5f - t * 0.5f, w - 2 * t, t }); // g
    g.setColour (dot ? colours::segOn : colours::segOff);
    g.fillEllipse (x + w + t * 0.4f, y + h - t, t, t);
}

void SegmentDisplay::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff2a0c0a));
    g.fillRoundedRectangle (b, 3.0f);
    auto inner = b.reduced (10.0f, 8.0f);
    const float digitW = (inner.getWidth() - 14.0f) / 2.0f - 6.0f;
    drawDigit (g, { inner.getX(), inner.getY(), digitW, inner.getHeight() }, text_[0], dots_);
    drawDigit (g, { inner.getX() + digitW + 14.0f, inner.getY(), digitW, inner.getHeight() }, text_[1], dots_);
}

// ---------------------------------------------------------------------------
void Led::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().withSizeKeepingCentre (7.0f, 7.0f);
    if (on_)
    {
        g.setColour (colours::ledOn.withAlpha (0.35f));
        g.fillEllipse (r.expanded (3.0f));
    }
    g.setColour (on_ ? colours::ledOn : colours::ledOff);
    g.fillEllipse (r);
}

// ---------------------------------------------------------------------------
PanelKnob::PanelKnob()
{
    setSliderStyle (juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
}

void PanelKnob::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    auto r = b.withSizeKeepingCentre (juce::jmin (b.getWidth(), b.getHeight()) - 8.0f, juce::jmin (b.getWidth(), b.getHeight()) - 8.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (r.translated (1.0f, 2.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a3a), r.getX(), r.getY(), juce::Colour (0xff121212), r.getX(), r.getBottom(), false));
    g.fillEllipse (r);
    // Ribs
    g.setColour (juce::Colour (0xff2a2a2a));
    for (int i = 0; i < 24; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * static_cast<float> (i) / 24.0f;
        g.drawLine (r.getCentreX() + std::cos (a) * r.getWidth() * 0.42f, r.getCentreY() + std::sin (a) * r.getWidth() * 0.42f,
                    r.getCentreX() + std::cos (a) * r.getWidth() * 0.5f, r.getCentreY() + std::sin (a) * r.getWidth() * 0.5f, 1.5f);
    }
    const double range = getMaximum() - getMinimum();
    const float pos = range > 0.0 ? static_cast<float> ((getValue() - getMinimum()) / range) : 0.0f;
    const float angle = juce::MathConstants<float>::pi * 1.25f + pos * juce::MathConstants<float>::pi * 1.5f;
    g.setColour (colours::ledOn);
    g.drawLine (r.getCentreX() + std::sin (angle) * r.getWidth() * 0.15f, r.getCentreY() - std::cos (angle) * r.getWidth() * 0.15f,
                r.getCentreX() + std::sin (angle) * r.getWidth() * 0.45f, r.getCentreY() - std::cos (angle) * r.getWidth() * 0.45f, 2.5f);
}

} // namespace jane60::ui

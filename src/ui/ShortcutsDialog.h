// SPDX-License-Identifier: GPL-3.0-or-later
// "Keyboard shortcuts..." (Settings > Extras): one window that lays out the computer-keyboard
// map in three groups, each row a key cap and what it does. Mirrors docs/user/keyboard-shortcuts.md.
#pragma once

#include <JuceHeader.h>

#include <vector>

namespace jane60::ui
{

class ShortcutsComponent final : public juce::Component
{
public:
    ShortcutsComponent()
    {
        close_.setButtonText ("Close");
        close_.onClick = [this]
        {
            if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
                dw->exitModalState (0);
        };
        addAndMakeVisible (close_);
        setSize (720, contentHeight() + 64);
    }

    static void show()
    {
        juce::DialogWindow::LaunchOptions o;
        o.content.setOwned (new ShortcutsComponent());
        o.dialogTitle = "Keyboard shortcuts";
        o.dialogBackgroundColour = juce::Colour (0xff181818);
        o.escapeKeyTriggersCloseButton = true;
        o.useNativeTitleBar = true;
        o.resizable = false;
        o.launchAsync();
    }

    void resized() override
    {
        close_.setBounds (getWidth() - 100 - kMargin, getHeight() - 32 - kMargin, 100, 32);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff181818));
        int y = kMargin;
        g.setColour (juce::Colour (0xfff3f1ea));
        g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
        g.drawText ("Keyboard shortcuts", kMargin, y, getWidth() - 2 * kMargin, 28, juce::Justification::centredLeft);
        y += 30;
        g.setColour (juce::Colour (0xff9a9a9a));
        g.setFont (juce::FontOptions (13.5f));
        g.drawFittedText ("They work in the standalone app, and in the plugin window once it has keyboard focus (click the panel or the on-screen keys). "
                          "A host may capture some keys; the standalone app is the reliable place.",
                          kMargin, y, getWidth() - 2 * kMargin, 36, juce::Justification::topLeft, 2);
        y += 44;

        for (const auto& group : groups())
        {
            g.setColour (juce::Colour (0xffff9f40));
            g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
            g.drawText (group.title.toUpperCase(), kMargin, y, getWidth() - 2 * kMargin, 20, juce::Justification::centredLeft);
            y += 22;
            g.setColour (juce::Colour (0xff333333));
            g.fillRect (kMargin, y, getWidth() - 2 * kMargin, 1);
            y += 6;
            for (const auto& row : group.rows)
            {
                const int h = rowHeight (row);
                drawKeys (g, row.keys, kMargin, y + 3);
                g.setColour (juce::Colour (0xffe6e6e6));
                g.setFont (juce::FontOptions (14.0f));
                g.drawFittedText (row.what, kMargin + kKeyColumn, y + 2, getWidth() - 2 * kMargin - kKeyColumn, h - 4,
                                  juce::Justification::topLeft, 2);
                y += h;
            }
            y += kGroupGap;
        }
    }

private:
    struct Row { juce::StringArray keys; juce::String what; };
    struct Group { juce::String title; std::vector<Row> rows; };

    static constexpr int kMargin = 22, kKeyColumn = 250, kGroupGap = 14, kCapH = 22;

    static const std::vector<Group>& groups()
    {
        static const std::vector<Group> g = {
            { "Playing notes", {
                { { "A", "S", "D", "F", "G", "H", "J", "K" }, "White keys of one octave, C to C" },
                { { "W", "E", "T", "Y", "U" }, "Black keys" },
                { { "Z", "X" }, "Octave down, octave up" },
                { { "Mouse" }, "Click the keys of the on-screen keyboard" } } },
            { "Audition helpers", {
                { { "1 to 7" }, "Diatonic triad on that degree of C major (C, Dm, Em, F, G, Am, Bdim) with a bass note an octave below, held 1.5 s" },
                { { "Shift", "1 to 7" }, "The same chord with its seventh" },
                { { "Space" }, "Start or stop a looping chord progression" },
                { { "P" }, "Next progression (Shift + P: previous). Pop, jazz, ballad, cadence, minor" },
                { { "[", "]" }, "Slower or faster loop, 0.4 to 6 s per chord" },
                { { "-", "=" }, "Transpose the chords down or up one semitone (C4 by default)" },
                { { "0" }, "Stop everything" } } },
            { "Panel", {
                { { "Shift", "click bank 1 or 2" }, "Bank 6 or 7 (the hardware holds bank 5 and presses 1 or 2)" },
                { { "Double-click a slider" }, "Reset it to 0" },
                { { "Mouse wheel over a slider" }, "Fine adjustment" },
                { { "MAN" }, "Manual: the display shows -- until a memory is selected; the panel is always live" },
                { { "WRITE", "bank", "patch" }, "Store the panel in that memory number (kept as a user preset)" } } } };
        return g;
    }

    static int rowHeight (const Row& r) { return r.what.length() > 70 ? 44 : 30; }

    static int contentHeight()
    {
        int h = kMargin + 30 + 44;
        for (const auto& group : groups())
        {
            h += 28;
            for (const auto& row : group.rows) h += rowHeight (row);
            h += kGroupGap;
        }
        return h;
    }

    static void drawKeys (juce::Graphics& g, const juce::StringArray& keys, int x, int y)
    {
        for (int i = 0; i < keys.size(); ++i)
        {
            const auto& k = keys[i];
            const bool cap = k.length() <= 7;
            g.setFont (juce::FontOptions (cap ? 13.0f : 12.5f, juce::Font::bold));
            const int w = cap ? juce::jmax (22, static_cast<int> (juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), k)) + 10)
                              : static_cast<int> (juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), k)) + 8;
            if (cap)
            {
                g.setColour (juce::Colour (0xffe9e6dc));
                g.fillRoundedRectangle (static_cast<float> (x), static_cast<float> (y), static_cast<float> (w), static_cast<float> (kCapH), 4.0f);
                g.setColour (juce::Colour (0xffb8b4a8));
                g.drawRoundedRectangle (static_cast<float> (x) + 0.5f, static_cast<float> (y) + 0.5f, static_cast<float> (w) - 1.0f, static_cast<float> (kCapH) - 1.0f, 4.0f, 1.0f);
                g.setColour (juce::Colour (0xff202020));
            }
            else
                g.setColour (juce::Colour (0xffcfcfcf));
            g.drawText (k, x, y, w, kCapH, juce::Justification::centred);
            x += w + (i + 1 < keys.size() ? 5 : 0);
            if (i + 1 < keys.size() && ! cap)
                x += 2;
        }
    }

    juce::TextButton close_;
};

} // namespace jane60::ui

// SPDX-License-Identifier: GPL-3.0-or-later
// Reference coordinates for the panel, measured from the owner's top-down photo
// (2000 px wide, panel strip from x = 120 to x = 1880) and the 1060 mm width.
// Everything is laid out in this space and scaled to the window.
#pragma once

#include <JuceHeader.h>

namespace jane60::ui::layout
{

constexpr int refWidth = 1900;      // whole editor
constexpr int refHeight = 700;

// Main panel strip
constexpr int panelX = 60, panelY = 20, panelW = 1780, panelH = 300;
constexpr int bandY = panelY, bandH = 26;        // section header band
constexpr int bodyY = bandY + bandH;             // control area

// Section x ranges in panel coordinates (photo x minus 120, plus panelX)
struct Section { const char* title; int x0, x1; int band; }; // band: 0 cream, 1 blue, 2 red
constexpr int creamBand = 0, blueBand = 1, redBand = 2;
inline const Section sections[] = {
    { "POWER",         panelX + 0,    panelX + 75,   creamBand },
    { "",              panelX + 75,   panelX + 200,  creamBand },   // KEY TRANSPOSE / HOLD (legends drawn per button)
    { "ARPEGGIO",      panelX + 200,  panelX + 375,  blueBand },
    { "LFO",           panelX + 375,  panelX + 465,  redBand },
    { "DCO",           panelX + 465,  panelX + 775,  redBand },
    { "HPF",           panelX + 775,  panelX + 830,  redBand },
    { "VCF",           panelX + 830,  panelX + 1045, redBand },
    { "VCA",           panelX + 1045, panelX + 1120, redBand },
    { "ENV",           panelX + 1120, panelX + 1265, redBand },
    { "CHORUS",        panelX + 1265, panelX + 1380, redBand },
    { "MEMORY",        panelX + 1380, panelX + 1760, blueBand },
};

// Slider geometry
constexpr int sliderW = 48, sliderTop = bodyY + 34, sliderH = 210;
inline juce::Rectangle<int> slider (int centreX) { return { centreX - sliderW / 2, sliderTop, sliderW, sliderH }; }

// Buttons
constexpr int btnW = 36, btnH = 28, btnLedH = 12;
inline juce::Rectangle<int> button (int centreX, int topY) { return { centreX - btnW / 2, topY, btnW, btnH + btnLedH }; }
constexpr int waveBtnY = bodyY + 70;   // DCO waveform / chorus buttons (one row)

// Switches
inline juce::Rectangle<int> vswitch (int centreX, int topY, int positions) { return { centreX - 12, topY, 78, 22 * positions + 8 }; }

// Bender panel (below the main panel, left)
constexpr int benderX = 60, benderY = 350, benderW = 290, benderH = 300;

// Keyboard (61 keys, C2..C7)
constexpr int keysX = 370, keysY = 360, keysW = 1470, keysH = 240;

// Modern strip
constexpr int stripY = 660, stripH = 36;

} // namespace jane60::ui::layout

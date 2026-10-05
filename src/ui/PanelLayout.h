// SPDX-License-Identifier: GPL-3.0-or-later
// Reference coordinates for the editor. The panel keeps the hardware's order and
// section colours, but every control gets its own column (laid out left to right
// in PanelComponent::buildMainPanel) so legends never collide. The editor scales
// the whole reference space to the window.
#pragma once

#include <JuceHeader.h>

namespace jane60::ui::layout
{

constexpr int refWidth = 2260;      // whole editor
constexpr int refHeight = 850;

// Main panel strip
constexpr int panelX = 60, panelY = 20, panelW = refWidth - 2 * panelX, panelH = 380;
constexpr int bandY = panelY, bandH = 30;        // section header band
constexpr int bodyY = bandY + bandH;             // control area
constexpr int sectionPad = 6;                    // inner margin at both ends of a section

// Fonts (reference points; the window scale applies on top)
constexpr float fontBand = 17.0f;      // section titles
constexpr float fontLegend = 14.0f;    // control legends
constexpr float fontSwitch = 11.0f;    // slide switch position legends
constexpr float fontScale = 9.5f;     // slider scale numbers
constexpr float fontCap = 14.0f;       // text printed on button caps

// Slider column: slot centred, scale numbers at the left, legend above
constexpr int sliderCol = 42;
constexpr int sliderTop = bodyY + 44, sliderH = 250;
inline juce::Rectangle<int> slider (int centreX) { return { centreX - sliderCol / 2, sliderTop, sliderCol, sliderH }; }
constexpr int legendH = 32;            // two-line legend box above sliders
inline juce::Rectangle<int> sliderLegend (int centreX) { return { centreX - sliderCol / 2 - 10, sliderTop - legendH - 4, sliderCol + 20, legendH }; }

// Buttons (36 x 28 cap with a 12 px LED strip above), one row
constexpr int btnW = 36, btnH = 28, btnLedH = 12, btnCol = 44;
constexpr int btnY = sliderTop + 96;
inline juce::Rectangle<int> button (int centreX) { return { centreX - btnW / 2, btnY, btnW, btnH + btnLedH }; }
inline juce::Rectangle<int> buttonLegend (int centreX, int w = 80) { return { centreX - w / 2, btnY - legendH - 2, w, legendH }; }

// Slide switches: slot at the left, position legends to the right, title above
constexpr int switchTop = sliderTop + 70;
inline juce::Rectangle<int> vswitch (int centreX, int width, int positions) { return { centreX - width / 2, switchTop, width, 24 * positions + 8 }; }
inline juce::Rectangle<int> switchLegend (int centreX, int w = 84) { return { centreX - w / 2, switchTop - legendH - 2, w, legendH }; }

// Memory section buttons (cap text printed on the button)
constexpr int memBtnW = 30, memBtnH = 26, memBtnCol = 34;

// Bender panel (below the main panel, left)
constexpr int benderX = 60, benderY = panelY + panelH + 30, benderW = 300, benderH = 370;

// Keyboard (61 keys, C2..C7)
constexpr int keysX = benderX + benderW + 20, keysY = benderY + 10, keysW = refWidth - keysX - panelX, keysH = 340;

// Modern strip
constexpr int stripY = benderY + benderH + 10, stripH = 40;

} // namespace jane60::ui::layout

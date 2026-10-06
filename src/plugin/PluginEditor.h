// SPDX-License-Identifier: GPL-3.0-or-later
// The editor: panel + bender panel + 61-key keyboard, all in reference
// coordinates scaled to the window, with a modern strip below.
#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "ui/PanelComponent.h"
#include "DemoPlayer.h"

#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

namespace jane60
{

class Jane60Editor final : public juce::AudioProcessorEditor,
                           private juce::Timer
{
public:
    explicit Jane60Editor (Jane60Processor&);
    ~Jane60Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;
    bool keyPressed (const juce::KeyPress& key) override;

private:
    void timerCallback() override;
    void refreshPresetList();
    void savePreset();
    /// The one settings menu: unit settings (condition layer), plus the Standalone's audio,
    /// state and reset items when the window is available.
    void showSettingsMenu (juce::Component* target, juce::StandaloneFilterWindow* window);

    Jane60Processor& processor_;

    // Everything in reference space lives inside content_, which is scaled as a whole.
    juce::Component content_;
    ui::PanelComponent panel_;
    juce::MidiKeyboardComponent keyboard_;
    DemoPlayer demo_;

    // Modern strip (also in reference space)
    juce::TextButton prev_ { "<" }, next_ { ">" }, ab_ { "A" }, copy_ { "Copy" }, save_ { "Save" }, undo_ { "Undo" }, settings_ { "Settings" };
    juce::ComboBox presetBox_;
    juce::Label edited_, extras_;
    int lastShownIndex_ = -1;

    // Standalone: replaces the window's title-bar "Options" button so there is one settings menu.
    juce::TextButton windowSettings_ { "Settings" };
    bool standaloneHooked_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Jane60Editor)
};

} // namespace jane60

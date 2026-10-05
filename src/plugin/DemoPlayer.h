// SPDX-License-Identifier: GPL-3.0-or-later
// Hidden test helper: plays diatonic chords and looping chord progressions
// into the keyboard state from computer-keyboard shortcuts, so sounds can be
// auditioned against reference recordings without a MIDI keyboard.
// Shortcuts are documented in docs/user/keyboard-shortcuts.md and deliberately
// not shown in the interface.
#pragma once

#include <JuceHeader.h>

#include <array>
#include <vector>

namespace jane60
{

class DemoPlayer final : private juce::Timer
{
public:
    explicit DemoPlayer (juce::MidiKeyboardState& state);
    ~DemoPlayer() override;

    /// Returns true if the key was handled.
    bool handleKey (const juce::KeyPress& key);

    bool isLooping() const noexcept { return looping_; }
    int progressionIndex() const noexcept { return progression_; }
    static juce::String progressionName (int index);

private:
    struct Chord { std::vector<int> notes; };

    void timerCallback() override;
    void playChord (const Chord& c, int holdMs);
    void stopChord();
    void startLoop();
    void stopLoop();
    Chord degreeChord (int degree, bool seventh) const;

    juce::MidiKeyboardState& state_;
    std::vector<int> sounding_;
    bool looping_ = false;
    int progression_ = 0;
    int step_ = 0;
    int root_ = 60;            // C4
    int chordMs_ = 1800;       // per chord in the loop
    int gateMs_ = 1500;        // held part of each step
    juce::int64 stepStartMs_ = 0;
    bool gateOpen_ = false;
    juce::int64 oneShotEndMs_ = 0;
};

} // namespace jane60

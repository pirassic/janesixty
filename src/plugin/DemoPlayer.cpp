// SPDX-License-Identifier: GPL-3.0-or-later

#include "DemoPlayer.h"

namespace jane60
{

namespace
{
// Degrees of the major scale as semitone offsets from the root.
constexpr int kMajor[7] = { 0, 2, 4, 5, 7, 9, 11 };

// Progressions as scale degrees (1-based). Negative = add a seventh.
struct Progression { const char* name; std::vector<int> degrees; };
const std::vector<Progression>& progressions()
{
    static const std::vector<Progression> p = {
        { "I V vi IV (pop)",            { 1, 5, 6, 4 } },
        { "ii V I vi (jazz-ish, 7ths)", { -2, -5, -1, -6 } },
        { "vi IV I V (ballad)",         { 6, 4, 1, 5 } },
        { "I IV V I (cadence)",         { 1, 4, 5, 1 } },
        { "i VI III VII (minor, A)",    { 6, 4, 1, 5 } },
    };
    return p;
}
} // namespace

DemoPlayer::DemoPlayer (juce::MidiKeyboardState& state) : state_ (state) {}

DemoPlayer::~DemoPlayer()
{
    stopTimer();
    stopChord();
}

juce::String DemoPlayer::progressionName (int index)
{
    const auto& p = progressions();
    if (index < 0 || index >= static_cast<int> (p.size())) return {};
    return p[static_cast<std::size_t> (index)].name;
}

DemoPlayer::Chord DemoPlayer::degreeChord (int degree, bool seventh) const
{
    // Diatonic triad on the degree (1..7) in the major scale of root_, close voicing
    // kept inside one octave above the root, with the bass an octave below.
    degree = ((degree - 1) % 7 + 7) % 7; // 0..6
    auto scaleNote = [&] (int idx) { return root_ + 12 * (idx / 7) + kMajor[idx % 7]; };
    Chord c;
    c.notes = { scaleNote (degree), scaleNote (degree + 2), scaleNote (degree + 4) };
    if (seventh) c.notes.push_back (scaleNote (degree + 6));
    // Bring the chord down so the top note stays below root + 16 semitones.
    while (c.notes.back() > root_ + 16)
        for (auto& n : c.notes) n -= 12;
    c.notes.insert (c.notes.begin(), c.notes.front() - 12); // bass
    return c;
}

void DemoPlayer::playChord (const Chord& c, int holdMs)
{
    stopChord();
    for (int n : c.notes)
    {
        state_.noteOn (1, n, 0.8f);
        sounding_.push_back (n);
    }
    gateOpen_ = true;
    if (holdMs > 0)
    {
        oneShotEndMs_ = juce::Time::currentTimeMillis() + holdMs;
        if (! looping_) startTimer (20);
    }
}

void DemoPlayer::stopChord()
{
    for (int n : sounding_)
        state_.noteOff (1, n, 0.0f);
    sounding_.clear();
    gateOpen_ = false;
}

void DemoPlayer::startLoop()
{
    looping_ = true;
    step_ = 0;
    stepStartMs_ = 0;
    startTimer (20);
}

void DemoPlayer::stopLoop()
{
    looping_ = false;
    stopChord();
    if (oneShotEndMs_ == 0) stopTimer();
}

void DemoPlayer::timerCallback()
{
    const auto now = juce::Time::currentTimeMillis();
    if (looping_)
    {
        const auto& prog = progressions()[static_cast<std::size_t> (progression_)];
        if (stepStartMs_ == 0 || now - stepStartMs_ >= chordMs_)
        {
            stepStartMs_ = now;
            const int d = prog.degrees[static_cast<std::size_t> (step_)];
            playChord (degreeChord (std::abs (d), d < 0), 0);
            step_ = (step_ + 1) % static_cast<int> (prog.degrees.size());
        }
        else if (gateOpen_ && now - stepStartMs_ >= gateMs_)
        {
            stopChord();
        }
        return;
    }
    if (oneShotEndMs_ != 0 && now >= oneShotEndMs_)
    {
        oneShotEndMs_ = 0;
        stopChord();
        stopTimer();
    }
}

bool DemoPlayer::handleKey (const juce::KeyPress& key)
{
    const auto ch = key.getTextCharacter();
    const bool shift = key.getModifiers().isShiftDown();

    if (ch >= '1' && ch <= '7')
    {
        playChord (degreeChord (ch - '0', shift), 1500);
        return true;
    }
    if (ch == '0')
    {
        stopLoop();
        stopChord();
        return true;
    }
    if (key.getKeyCode() == juce::KeyPress::spaceKey)
    {
        if (looping_) stopLoop(); else startLoop();
        return true;
    }
    if (ch == 'p' || ch == 'P')
    {
        progression_ = (progression_ + (shift ? static_cast<int> (progressions().size()) - 1 : 1)) % static_cast<int> (progressions().size());
        step_ = 0;
        stepStartMs_ = 0;
        return true;
    }
    if (ch == '[' || ch == ']')
    {
        chordMs_ = juce::jlimit (400, 6000, chordMs_ + (ch == '[' ? -200 : 200));
        gateMs_ = static_cast<int> (chordMs_ * 0.83);
        return true;
    }
    if (ch == '-' || ch == '=')
    {
        root_ = juce::jlimit (36, 84, root_ + (ch == '-' ? -1 : 1));
        return true;
    }
    return false;
}

} // namespace jane60

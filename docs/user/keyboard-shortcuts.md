# Keyboard shortcuts

These work in the standalone app and in the plugin window when it has keyboard focus (click the on-screen keyboard or the panel first). Inside a DAW the host may capture some keys; the standalone app is the reliable place.

## Playing notes

| Keys | Action |
|---|---|
| A S D F G H J K | White keys of one octave (C to C) |
| W E T Y U | Black keys |
| Z / X | Octave down / up |
| Mouse | Click the keys of the on-screen keyboard |

## Audition helpers (not shown in the interface)

Meant for checking sounds against reference recordings without a MIDI keyboard. They play into the same keyboard input as the on-screen keys, so HOLD, the arpeggiator and key transpose all apply.

| Keys | Action |
|---|---|
| 1 to 7 | Play the diatonic triad on that degree of C major (C, Dm, Em, F, G, Am, Bdim) with a bass note an octave below, held for 1.5 s |
| Shift + 1 to 7 | Same with the seventh added |
| Space | Start / stop a looping chord progression |
| P | Next progression (Shift + P: previous) |
| [ / ] | Slower / faster loop (200 ms per step, 0.4 to 6 s per chord) |
| - / = | Transpose the demo root down / up one semitone (C4 by default) |
| 0 | Stop everything |

Progressions, in order:

1. I V vi IV (pop): C, G, Am, F
2. ii V I vi with sevenths (jazz-ish): Dm7, G7, Cmaj7, Am7
3. vi IV I V (ballad): Am, F, C, G
4. I IV V I (cadence): C, F, G, C
5. i VI III VII (minor, same notes as 3 heard from A minor)

Each chord in a loop is held for about 83 % of the step, so patches with a release tail and the arpeggiator both read correctly.

## Panel

| Keys | Action |
|---|---|
| Shift + click bank 1 or 2 | Bank 6 or 7 (hardware: hold bank 5 and press 1 or 2) |
| MAN | Manual: the display shows "--" until a memory is selected (the panel is always live in software) |
| WRITE, then a bank and a patch button | Store the current panel in that memory number (kept as a user preset in the Memory bank) |
| Double-click a slider | Reset it to 0 |
| Mouse wheel over a slider | Fine adjust |

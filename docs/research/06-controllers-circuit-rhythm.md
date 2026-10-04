# Pad controllers for the performance layer: Novation Circuit Rhythm findings

Read first-hand on 2026-10-04: Circuit Rhythm User Guide v1.0 (95 pages), Circuit Rhythm 2.0 firmware addendum (11 pages), Circuit Rhythm Programmer's Reference Guide v5 (6 pages).

## 1. What the Circuit Rhythm can do as a controller

- **Pads send MIDI notes with velocity.** The grid is "32 illuminated, velocity-sensitive pads, which act as a chromatic keyboard" (UG p.5). In Note View the lower 16 pads are one chromatic octave (white notes bottom row, black notes above, C at bottom-left = middle C by default); Expanded Note View turns the upper 16 pads into a second octave (UG p.27 to 28). The octave arrows shift the keyboard up or down by one to five octaves (UG p.33).
- **Each track transmits on its own MIDI channel** (defaults: tracks 1 to 8 on channels 1 to 8; channel 16 reserved for the project), changeable in Setup View (UG p.89). MIDI Note Tx is on by default and can be switched independently of CC, Program Change and Clock (UG p.90).
- **Velocity is variable by default** (how hard the pad is struck); Fixed Velocity (96) is a global toggle (UG p.39).
- **Ports:** USB-C class-compliant MIDI plus 5-pin In/Out/Thru (UG p.17). MIDI Thru can clone Out.
- **Sample trigger notes** (from the programmer's reference p.5) are the notes the Rhythm *receives* on channel 16 to fire its own sample tracks (C1 36 to C2 48). They do not describe what the pads send.
- **Grid FX** are controllable over MIDI CC (programmer's reference p.4).

## 2. What it cannot do

- **No LED control from the host.** The programmer's reference v5 lists only CC and NRPN parameters, program change, realtime and system common messages. There is no note-to-LED mapping, no SysEx colour message, and no "programmer mode" like the Launchpad family. The user guide's only mention of a computer talking to the unit is Novation Components (sample and project management over Web MIDI). The pad colours are always the Rhythm's own: track colour for the keyboard, bright for the currently selected note, step colours in the sequencer rows.
- So **scale and chord degrees cannot be painted onto the Circuit Rhythm's pads**. The unit will show its fixed chromatic keyboard while our software remaps what each pad does. That is usable (the layout is still a keyboard, and the software can show the mapping on screen) but it is not the Polyend experience, where the pads themselves show the scale.
- There is no note-mode "scale" setting on the Rhythm itself (Circuit Tracks has scales for its synth tracks; the Rhythm's Note View is chromatic only, UG p.33).

## 3. Consequences for the design

1. Build the performance layer around an **abstract grid input** (pad index, velocity, on/off) and an **optional abstract grid display** (set pad colour). The Circuit Rhythm implements the input side only. Any controller with a programmer mode implements both.
2. Provide **on-screen feedback** in the plugin and standalone: a drawn grid that mirrors the physical pads, showing scale degrees, chord roots and the currently sounding notes. This gives the Circuit Rhythm user the missing visual layer, and gives Mac-keyboard users the same view.
3. For true LED feedback, support a controller with a documented LED protocol as the second target. Known documented options (not yet verified in this session; the agent that checked public sources could not reach Novation's site):
   - Launchpad Mini MK3 / Launchpad X / Launchpad Pro MK3: "Programmer mode" switched by a short SysEx, then note-on velocity = colour index on pads, with SysEx RGB and flash/pulse messages. Novation publishes a Programmer's Reference for each.
   - Ableton Move: has a controller mode; its MIDI/LED protocol is less openly documented.
   - Any Launchpad is the lowest-risk choice if the owner wants the Polyend-style lit grid.
4. Circuit Rhythm mapping defaults for our layer: listen on the channel of the track the user selects (default track 1, channel 1), treat the 16 or 32 Note View pads as the pad grid in keyboard order (C = pad 0 ... B = pad 11 within an octave, 2 octaves in Expanded Note View), use incoming velocity when the velocity extra is enabled, and let the Rhythm's own octave arrows shift the range (we remap relative to the received note number, so octave shifts carry through).

## 4. Mac keyboard input (standalone app)

- JUCE delivers key down/up events to the focused window; the standalone app can run the performance grid from two keyboard rows (for example Z..M and Q..P as two rows of up to 10 pads each, or a single row of 12 for one octave).
- Limits: most keyboards report at most 6 simultaneous keys over USB HID (6-key rollover) plus modifiers, and key repeat must be suppressed (JUCE reports repeats as additional keyDown calls). Six simultaneous keys matches the Juno's six voices, so this is not a practical problem for scale mode; for chord mode one key is one chord.
- No velocity from a computer keyboard; the layer sends a fixed velocity (configurable), which only matters when the velocity extra is on.
- Inside a DAW the host usually captures computer-keyboard input, so the Mac keyboard path is standalone-first by design; in a DAW the same transform acts on whatever notes the host sends (for example Logic's musical typing).

## 5. LED-capable alternatives (from a parallel check of public sources)

**Launchpad Mini MK3**, protocol verified from the official Programmer's Reference (copy obtained from a GitHub mirror):
- Two USB MIDI ports, "LPMiniMK3 DAW" and "LPMiniMK3 MIDI"; programmer mode uses the MIDI port.
- Enter programmer mode: `F0 00 20 29 02 0D 0E 01 F7` (`00` returns to Live mode).
- Light pads with plain Note On on the MIDI port: channel 1 static, channel 2 flashing, channel 3 pulsing; note number = pad in programmer layout (row x 10 + column, 11 = bottom left, 88 = top right); velocity = palette index 0 to 127 (5 red, 13 yellow, 21 green, 45 blue); Note Off or velocity 0 = off.
- RGB SysEx: `F0 00 20 29 02 0D 03 <type> <led> <data...> F7`, type 0 palette, 1 flash, 2 pulse, 3 RGB (3 bytes 0 to 127); up to 81 LEDs per message.
- Pads send Note On/Off; the Mini MK3 pads are not velocity sensitive (reported, unverified). Launchpad X (device byte `0C`) and Pro MK3 (`0E`) use the same message family with velocity and polyphonic aftertouch (not byte-verified here).

**Ableton Move**: no official controller mode or programmer reference; Ableton states MIDI does not work in Control Live mode. A reverse-engineered control-surface protocol exists (auridevil/movewire, MIT; awwbees/MoveControlHost): pads send notes 68 to 99 on channel 1 with velocity and aftertouch, LEDs are set by sending the same note back with velocity = palette index, and a keep-alive is needed. Unofficial, firmware-dependent, not a v1 target.

Recommendation for the performance layer: Circuit Rhythm as a one-way pad source (what the owner has), Launchpad programmer mode as the first LED-feedback target (documented, trivial protocol), on-screen grid for everyone.

## 6. Panel photograph sources (network-blocked from this session, to fetch from the owner's machine)

- Wikimedia Commons `File:Roland Juno-60 (6935222973).jpg`, reported 2048 x 741 px, CC BY 2.0 (Flickr user deepsonic). Aspect suggests a straight-on whole-instrument view; verify.
- Wikimedia Commons `File:Roland Juno-60.jpg`, 1200 x 440 px, CC BY-SA 2.0 (the Wikipedia article image).
- Commons categories `Category:Roland Juno 60` and `Category:Roland Juno-60 (JU-60) (serial no. 357180)` reportedly hold further front-panel and detail photos up to 2048 x 1536 px; licence per file.
- Audiofanzine gallery (user-uploaded, copyrighted): fine as a private measurement reference, never redistributed.

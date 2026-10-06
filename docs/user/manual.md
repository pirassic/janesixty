---
title: Jane-Sixty User Manual
subtitle: A component-modelled Roland Juno-60 for macOS and Windows
---

![](../../assets/icon/jane60-256.png){width=96px .icon}

# Jane-Sixty

Jane-Sixty recreates the Roland Juno-60 polyphonic synthesizer of 1982 as an Audio Unit, a VST3 plugin and a standalone application. It is a component model: each part of the original circuit, from the digitally controlled oscillators through the IR3109 filter, the envelopes and the bucket-brigade chorus, is modelled from the Service Notes, the schematics, chip datasheets and published measurements, and the numbers behind every block are kept in a calibration file with their sources. The panel keeps the hardware's layout, names and six-voice polyphony.

Jane-Sixty is free software, published under the GNU General Public License version 3. The source, the research notes and the calibration data are public, and contributions are welcome (see the last section).

Roland and Juno are trademarks of Roland Corporation. This software is not affiliated with or endorsed by Roland.

# Installing

## macOS

Download `Jane-Sixty-x.y.z-macOS.pkg` from the release page and open it. The installer is signed and notarised, so macOS opens it without a warning. On the component page you can deselect what you do not need:

- **Audio Unit** into `/Library/Audio/Plug-Ins/Components`, for Logic Pro, GarageBand and MainStage.
- **VST3** into `/Library/Audio/Plug-Ins/VST3`, for Live, Cubase, Reaper, Bitwig, Studio One and most others.
- **Standalone application** into `/Applications`.

The installer needs macOS 11 or later and runs natively on Apple silicon and Intel Macs.

If you download one of the zipped bundles instead of the installer, macOS quarantines it. Either open it once through System Settings, Privacy and Security, or remove the quarantine in Terminal:

```
xattr -dr com.apple.quarantine ~/Downloads/Jane-Sixty.vst3
```

## Windows

Run `Jane-Sixty-x.y.z-win64-setup.exe`. It installs the VST3 into `C:\Program Files\Common Files\VST3` and the standalone application into `C:\Program Files\clevergear\Jane-Sixty`, with a Start Menu entry and an uninstaller. The installer is not signed, so SmartScreen may ask you to confirm the first time. Windows 10 or later, 64-bit.

## In a host

After installing, rescan plugins in your host if it does not pick Jane-Sixty up by itself. The plugin is an instrument: put it on an instrument or MIDI track. It takes MIDI notes, pitch bend, the sustain pedal (CC 64) and program changes (the 56 factory patches are exposed as programs 1 to 56).

## Standalone

The standalone application plays from a MIDI keyboard, the on-screen keyboard and the computer keyboard. Audio and MIDI devices are set from the **Settings** button at the top left of the window ("Audio/MIDI settings..."). The same menu saves and loads the whole state of the instrument to a file.

# A first sound

1. Choose a patch from the preset box at the bottom, or with the BANK and PATCH NUMBER buttons: bank 1, patch 1 is Strings 1, the sound most people know the Juno-60 for.
2. Play. In the standalone the keys `A S D F G H J K` play an octave, `W E T Y U` the sharps, `Z` and `X` move the octave. Click the on-screen keyboard or the panel first if the keys do nothing.
3. Switch the CHORUS between OFF, I and II. The chorus is where much of the instrument's character lives.
4. Move the VCF FREQ slider to open and close the filter, and RES to make it sing. These two sliders and the ENV slider next to them are where most Juno patches are made.

# The panel

The panel follows the hardware from left to right. Sliders run from 0 at the bottom to 10 at the top; the VCA LEVEL slider runs from -5 to +5 with 0 as the nominal level. Double-click a slider to reset it, use the mouse wheel over it for fine steps.

## POWER

The switch is decorative. The instrument is always on.

## KEY TRANSPOSE and HOLD

**KEY TRANSPOSE** shifts the keyboard by up to eleven semitones upward (the hardware transposes within the octave; the top C gives one octave). Click the button, then press a key on the on-screen keyboard: that key becomes C. Click the button again to return.

**HOLD** keeps notes sounding after the keys are released, like a latch. With more than six keys, the last six stay. The sustain pedal (CC 64) does the same from a MIDI controller.

## ARPEGGIO

ON/OFF switches the arpeggiator in, MODE chooses UP, UP & DOWN or DOWN, RANGE spans one, two or three octaves above the keys held, RATE sets the speed from about 1.5 to 50 steps per second. Hold a chord with HOLD on and the arpeggio keeps running.

## LFO

One triangle LFO shared by the six voices. **RATE** runs from about 0.3 Hz to 22 Hz. **DELAY TIME** holds the LFO back after a new phrase begins and fades it in over up to two seconds, so vibrato arrives after the attack. The **TRIG** switch chooses AUTO (every new phrase restarts the delay) or MAN (the LFO only runs while the LFO TRIG button on the bender panel is held).

## DCO

The digitally controlled oscillator, one per voice, all locked to one master clock, which is why the Juno never drifts and why its unison is clean.

- **LFO** sets vibrato depth; **PWM** sets the pulse width, or the depth of its modulation, depending on **PWM MODE**: LFO (the LFO moves the width), MAN (the slider sets a fixed width from 50 % to 95 %), ENV (the envelope moves it).
- The three buttons switch the **pulse** wave, the **sawtooth** and the **sub oscillator**, a square wave one octave below.
- **SUB OSC** sets the sub level, **NOISE** mixes in white noise.

## HPF

The high-pass filter sits after the voices, on the chorus board. Position 0 is flat; 1, 2 and 3 thin the low end progressively. Many factory patches use 1 to keep chords clear.

## VCF

The four-pole low-pass filter, the IR3109, the heart of the sound.

- **FREQ** sets the cutoff; the slider covers more than thirteen octaves, so most patches sit between 2 and 7.
- **RES** sets resonance. Near the top the filter self-oscillates, which bank 7's effect patches use. As on the hardware, the low end thins a little at high resonance.
- **ENV** sets how far the envelope opens the filter; **ENV POL** chooses whether the envelope pushes the cutoff up (NORM) or down (INV).
- **LFO** sets how far the LFO moves the cutoff.
- **KYBD** sets key tracking: at 10 the cutoff follows the keyboard one octave per octave, pivoting at middle C.

## VCA

**MODE** chooses ENV, the envelope shapes the loudness, or GATE, the note is on at full level while the key is held, as a fast organ-style gate. **LEVEL** is the patch's own level, from -5 to +5. It sits before the chorus, as on the hardware: pushing it hard drives the chorus's bucket-brigade delay into its soft overload, which some players use on purpose.

## ENV

A, D, S, R: attack from 1 ms to 3 seconds, decay and release from 2 ms to 12 seconds, sustain level. One envelope per voice, shared between the filter and the amplifier as on the hardware. A new note restarts the attack from the current level, so retriggered notes do not click.

## CHORUS

OFF, I, II, or both buttons for the hidden third mode. Chorus I is slow and wide, II is faster, I+II is a fast, nearly mono vibrato. The chorus runs on two bucket-brigade delays with no noise reduction, so there is a faint constant hiss, as on every real unit. Hot patches overload its input; see VCA LEVEL.

## MEMORY

The hardware's 56 memories are the factory patches, in seven banks of eight. Press a **BANK** button (1 to 5; Shift-click 1 or 2 for banks 6 and 7, which the hardware reaches by holding bank 5) and a **PATCH NUMBER**. The display shows the number.

- **MAN** (manual): the panel's own settings play; the display shows two dashes until a memory is selected. In software the panel is always live, so MAN only resets the display.
- **WRITE**: press WRITE, then a bank and a patch number, to store the current panel in that number. Stored patches live in your user preset folder in a bank called Memory and take precedence over the factory patch of that number.
- **TAPE / FILE**: the hardware saved memories to cassette. Here **SAVE** writes the current patch to a `.json` file, **LOAD** reads one, **VER** (verify) only flashes its light, since there is no tape to verify.

## The bender panel

**DCO** and **VCF** set how far the bender moves pitch (up to seven semitones) and cutoff. The **BENDER** lever snaps back to centre. **LFO TRIG** starts the LFO while held when TRIG is set to MAN. **OCTAVE TRANSPOSE** shifts the keyboard an octave down or up. **VOLUME** is the master output.

# Presets

The strip under the keyboard holds the preset box and the preset tools.

- The box lists the 56 factory patches by number and name, then your user presets grouped by bank (folder). The `<` and `>` buttons step through them.
- **A / B** swaps between two working slots, so a sound can be compared with a variation; **Copy** copies the active slot into the other one.
- **Save** stores the panel as a user preset, asking for a name and a bank. Presets are plain JSON files, in `~/Library/Audio/Presets/clevergear/Jane-Sixty` on macOS and `%APPDATA%\clevergear\Jane-Sixty` on Windows, so they can be shared and kept in version control.
- **Undo** reverts the last panel change.
- The word "edited" appears when the panel differs from the loaded preset.

A preset holds the panel and nothing else. Settings (next section) and the extras are never part of a preset, so a preset sounds the same relative to its panel on any setting.

# Settings

One **Settings** menu holds everything that is not a panel control. In the standalone it is the button at the top left of the window; in a host it is the Settings button at the right of the preset strip. Its choices are saved with the plugin instance (the project, or the standalone's state) and never in presets.

**Unit** describes the particular Juno-60 being emulated. Each entry is a documented uncertainty of the model, with the default taken from the best evidence:

| setting | default | what it does |
|---|---|---|
| Match the factory demo unit | on | The unit in Roland's factory demo recording has its filter trimmed one octave above the Service Notes. Off restores the manual's trim and makes every patch an octave darker. |
| Voice spread | Service Notes tolerances | Small fixed differences between the six voices, within the tolerances the service procedure allows. Off makes the six identical; twice makes a worn unit. |
| Filter drive | as calibrated | How hard the signal drives the filter's transistors. The default follows the schematic; the two alternatives change the filter's growl without changing levels. |
| Chorus noise | as calibrated | The bucket-brigade hiss at the chip's typical level; off, or the level of aged chips. |

**Extras** are things the hardware never had, off by default; when off the instrument is bit-for-bit the plain model:

- **Velocity** to VCA level, to filter envelope depth or to both, at 50 % or 100 %, with an optional soft curve.
- **MPE**: on an MPE controller each note gets its own pitch bend (48 semitones at full) and its own pressure, routed to the VCA, the filter envelope or both. Channel 1 stays the ordinary bender.
- **Keyboard shortcuts...** opens the list of computer-keyboard shortcuts, including the audition helpers that play chords and progressions for comparing sounds.

A small "VEL" or "MPE" badge in the preset strip shows while an extra is on.

In the standalone the same menu also holds **Audio/MIDI settings**, **Save current state**, **Load a saved state** and **Reset to default state**.

# MIDI

- Notes on any channel (channels 2 to 16 become per-note channels when MPE is on).
- Pitch bend acts as the bender lever, so its reach follows the bender panel's DCO and VCF sliders.
- CC 64, sustain pedal, is the hardware's PEDAL HOLD jack.
- Program change 1 to 56 recalls the factory patches, and the host's own preset mechanism recalls everything else.
- Velocity and channel pressure are ignored unless the extras are on, exactly like the hardware.

# How faithful is it

Jane-Sixty is modelled from documents, not from a unit on a bench, and says so. The master clock, waveform levels, filter anchor points, envelope and LFO ranges and the chorus bias point are the Service Notes' own calibration targets, so a correctly serviced Juno-60 matches to within that procedure's tolerances. The filter, the resonance network, the voice summer and the chorus board are read from the schematic and cross-checked against a circuit simulation in the project's continuous integration. The envelope, LFO and chorus delay tables, the bucket-brigade's gain, bandwidth and noise come from published measurements and the chip datasheets. The remaining unknowns, the unit's trim, the filter's drive, per-voice tolerances and the hiss of a particular unit, are the Settings above. The full accounting, every constant with its source and every assumed value with its uncertainty, is in `docs/calibration-report.md` in the repository.

If you can put a real Juno-60 on a bench, even for ten minutes, the bench protocol in `docs/research/08-bench-protocol.md` says what to capture; a handful of recordings would settle most of what is still assumed.

# Troubleshooting

- **No sound in the standalone**: open Settings, Audio/MIDI settings, and choose an output device and a MIDI input. Click the panel or the keys before using the computer keyboard.
- **The host does not show the plugin**: rescan plugins; on macOS, open Logic's Plug-in Manager and reset and rescan the Jane-Sixty entry.
- **A zipped download will not open on macOS**: it is quarantined; use the installer, or the `xattr` command above.
- **Bank 7 patches start quietly and build up**: those patches use the filter's self-oscillation, which grows from the noise floor over a second or two on the hardware too.
- **Everything is an octave darker than the demo videos**: Settings, "Match the factory demo unit" is off.

# Contributing

Jane-Sixty is open source under GPLv3, developed in the open at `github.com/pirassic/jnsynth`, and contributions are welcome: bug reports, listening notes against real units, bench measurements, schematic readings, code and documentation. The most valuable contribution of all is a capture from a real Juno-60; the research folder says exactly what to record and why.

To build from source you need CMake 3.25, a C++20 compiler and, for the plugin, the JUCE submodule; the README has the commands for macOS, Windows and Linux. The DSP has no framework dependency and its tests run on Linux in under a minute. Changes to the sound model should come with a source for every number, and go into the calibration file rather than the code wherever possible; the calibration report explains the tagging. Open an issue to discuss anything larger than a fix before building it, so the work lands.

See `CONTRIBUTING.md` in the repository for the practical details.

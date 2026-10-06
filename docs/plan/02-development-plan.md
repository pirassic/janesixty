# Development plan

Scope (decided 2026-10-04): Juno-60 only; AU, VST3 and standalone on macOS; no reference hardware; opt-in velocity and MPE; a Polyend-style performance layer as a late phase.

## 1. Stack

| Concern | Choice | Why |
|---|---|---|
| Framework | JUCE 9, used under AGPLv3 | mature AU + VST3 + standalone, accessibility, HiDPI, SVG |
| Project licence | GPLv3 | compatible with JUCE AGPL and with the GPL DSP libraries worth reusing |
| Build | CMake 3.25+, pamplejuce layout, Ninja | CI-friendly, universal binaries |
| Language | C++20 | |
| Tests | Catch2 v3, pluginval, auval, golden-render tests, simulation cross-checks | |
| Extra format | CLAP via clap-juce-extensions if JUCE 9 compatible | free, Bitwig and REAPER users |
| Optional libs | chowdsp_wdf (BSD-3), HIIR (WTFPL), chowdsp_utils BSD modules; GPL chowdsp DSP modules and jpcima BBD code where they save time | licence compatible |
| Offline sim and fitting | Python 3 (numpy, scipy, soundfile, matplotlib), ngspice or ACME.jl | |
| CI | GitHub Actions macos-15 (arm64) building universal; macos-15-intel for Intel-side tests | free for public repos |

Naming: **Jane-Sixty**, vendor **clevergear** (display name changeable at any time; the bundle id `com.clevergear.jane60` and the four-character manufacturer and plugin codes are frozen at v1.0 because hosts identify the plugin by them), `jane60` identifiers, no Roland or Juno mark anywhere in code, bundle id, 4-character codes or artwork.

## 2. Architecture

```
janesixty/
  CMakeLists.txt
  cmake/                 toolchain, signing, pluginval helpers
  libs/                  JUCE and third-party as submodules
  calibration/           juno60.json (constants with source tags; see research plan)
  src/
    dsp/                 framework-free, testable without JUCE
      core/              MasterClock (LC osc model with bend/LFO/tune in cents), Pitch (8253 integer periods), tables, oversampling
      dco/               Dco (integrator, finite-slope reset, 7-bit anti-log CV staircase), Waveshaper (saw, pulse comparator, sub flip-flop), Noise (shared)
      mixer/             Mixer (resistive sum, tapers, drive into the IR3109 attenuator)
      vcf/               Ir3109 (4-stage nonlinear ZDF OTA cascade + BA662 feedback with input-side Q comp), CutoffCvMix (FREQ, ENV x polarity, LFO, KYBD pivot C4, pedal offset, bender)
      vca/               Ba662Vca (ENV/GATE, 3/6 ms gate edges, offset thump), LevelVca (uPC1252H2, before chorus)
      env/               Ir3r01Envelope (stepped exponential, decay independent of sustain, retrigger from current level)
      lfo/               Lfo (free-running triangle, delay hold + fade, AUTO/MAN semantics)
      hpf/               Hpf (4 positions, position 0 flat)
      chorus/            BbdLine (Holters-Parker variable rate), ChorusBoard (pre/post filters, 3 modes, R inverted, mute fade, summers)
      voice/             Voice (one of six), VoiceAllocator (rotary default; non-rotary and unison as hidden test modes)
      perf/              Arpeggiator, Hold, KeyTranspose, Bender, OctaveTranspose
      Synth.h            six voices -> sum -> HPF -> level VCA -> chorus -> volume
      Tolerances.h       per-voice "condition" offsets
    plugin/              JUCE AudioProcessor, parameter layout, state, MIDI handling, extras (velocity, MPE)
    presets/             preset model, JSON format, factory bank, A/B, undo, Juno-106 .syx import (read-only convenience)
    midi/                CC map, NRPN, MIDI learn, program change, UMP
    performer/           scale mode, chord mode, grid input abstraction, grid display abstraction, controller drivers (phase 7)
    ui/                  panel renderer, controls, LookAndFeel, display, preset browser, settings, performer view
    standalone/          audio and MIDI device settings, computer-keyboard input
  tests/
    unit/                per-block Catch2 tests against calibration tables and simulation references
    golden/              offline renders of fixed MIDI vs committed WAVs
  tools/
    sim/                 ngspice / ACME netlists and reference-output scripts
    fit/                 scripts that turn simulation outputs and published tables into calibration/juno60.json
    panel/               SVG panel source and asset export
  docs/
```

Design rules:

- `src/dsp` has no JUCE dependency so blocks can be unit-tested and compared to simulation output in milliseconds.
- Every hardware constant lives in `calibration/juno60.json` with a source tag and is loaded into a `Calibration` struct; tests and DSP share the file. No magic numbers in DSP code.
- Per-voice tolerance is a separate layer on top of the calibrated nominal model; "factory fresh" is one switch.
- Internal rate: host rate, 2x oversampling around the IR3109 and the BBD line only; the DCO renders band-limited directly (BLEP at the reset, BLAMP at the slope change).
- Control signals (envelopes, LFO, CVs) are per-sample for the filter and VCA, because the hardware is continuous there. Panel sliders are quantised to 8 bits before use, as the hardware does, so the quantisation is reproduced rather than idealised away.
- Parameter IDs are strings fixed at v1 and never reordered.

## 3. DSP block specifications

Each block gets a one-page spec in `docs/spec/circuit/` before coding. Modelling approach per block:

### Master clock and pitch
One master clock shared by all voices, nominal 1 902 810 Hz, modulated in cents by bender (+-700 at full), LFO (+-300 at full) and tune (+-50). Per voice: integer divisor N = round(f_clock / f_note) with the keyboard table at A4 = 442 Hz by default shifted to 440 in the plugin (user-selectable). Pitch quantisation at the top of the range is reproduced.

### DCO
Falling integrator ramp 0 to -12 V equivalent; amplitude from the 7-bit anti-log CV staircase (one step per key, scaled with bend and LFO as the hardware does, so vibrato leaves amplitude exactly compensated); finite-slope flyback of simulated duration; optional incomplete-discharge offset. Pulse: comparator against the PWM CV with the -6.5 V (50 %) to -0.5 V (97 %) law, no level compensation, BLEP edges; sources MAN / LFO / ENV. Sub: divide-by-two square phase-locked to the reset. Noise: one shared generator (2SC945 junction model: white with the measured roll-off, plus an optional supply-ripple component for "condition").

### Mixer
Resistive sum with the schematic's leg resistors; saw and pulse as switches, sub and noise through their slider tapers; output scaled to the drive into the 68 k / 560 R attenuator.

### VCF
Four TPT one-pole OTA-C stages with tanh in each stage, global inverting feedback through a BA662 model with input-side Q compensation (coefficient from the schematic, 0.308 per the Juno-6 reading until the Juno-60 values are read), Newton iteration at 2x oversampling. Validated against ngspice. Cutoff CV: FREQ taper from simulation, ENV depth reaching 30 kHz at full from FREQ 0, LFO depth +-3.3 octaves at full, KYBD 1 oct/oct at 10 pivoting at C4, VCF pedal jack 3 V default offset, bender VCF depth. Resonance: 4 Vp-p self-oscillation at full, onset from simulation. Per-voice cutoff and resonance offsets in the tolerance layer.

### HPF
One-pole RC per the chorus-board network, four positions, position 0 flat.

### VCA and level
BA662 gain linear in control current, ENV or GATE with the measured 3 ms / 6 ms gate edges, residual offset thump term per voice. Shared LEVEL VCA (uPC1252H2) before the chorus with the slider taper from simulation.

### Envelope
IR3R01: stepped-oscillator timing with attack as a truncated exponential (reaches 1.0 at one time constant of a 1.58x target), decay and release exponential with duration independent of sustain, slider-to-time tables anchored at 1 ms / 3 s (A) and 2 ms / 12 s (D, R) with the published mid-points, sustain law from the published measurement, retrigger from current level, gate and trigger tied. All four sliders quantised to 8 bits.

### LFO
Free-running triangle, 0.3 to 22 Hz with the published rate table; delay as hold then fade; AUTO: delay restarts on the first key of a phrase; MAN: modulation only while LFO TRIG is held, delay ramp from the press. Destinations: master clock, cutoff CV mix, PWM.

### Chorus
Input gain, pre-filter chain (two Sallen-Key pairs plus the real pole, values from the schematic, response settled by simulation), two BBD lines (256 stages, variable clock from the simulated clock-oscillator law, transfer loss, saturation, noise floor and clock residue from published data), post filters, JFET mute with fade, summers (47 k dry, 39 k wet into 10 k). Modes I / II / I+II with 0.513 / 0.863 / 9.75 Hz and the published delay ranges; right channel inverted in I and II, in phase for I+II.

### Voice allocation and performance
Rotary assignment, steal the oldest voice; non-rotary and unison as hidden modes (they exist in the hardware test mode). HOLD (last six keys, needs sustain above 0). Arpeggiator UP / U&D / DOWN, 1 to 3 octaves, 1.5 to 50 Hz, host-sync option in place of the clock jack, the manual's turnaround and range-overflow rules. Key transpose (upward within the octave, top C = +12, blocked while the arpeggio runs). Octave transpose stored in the patch. Bender with DCO and VCF depths.

## 4. Plugin layer

- Parameters: one `AudioProcessorValueTreeState` parameter per panel control; discrete controls as choices; LEDs driven from parameters.
- State: APVTS XML + preset name + A/B slots + MIDI map + UI size + tolerance and extras settings, versioned.
- Presets: JSON with schema version; factory bank in BinaryData; user folder under ~/Library/Audio/Presets; browser with banks, search, tags, favourites; A/B with copy; undo/redo; "modified" indicator; import of the 56 factory patches from the chart CSV; Juno-106 .syx import as a convenience (mapped onto 60 parameters, 106-only fields dropped).
- MIDI: note on/off, pitch bend with range, mod wheel to LFO depth, CC64 hold, CC for every panel control with a published default map, 14-bit CC pairs, NRPN, MIDI learn, program change plus bank select, UMP accepted, panic, activity LED. Hardware behaviours preserved: no velocity, channel-wide bend, six-voice rotary stealing.
- **Extras (settings toggles, off by default):**
  - Velocity: destinations VCA level, VCF ENV depth, both; amount 0 to 100 %; curve linear or soft. When off, every note plays at full level as on the hardware.
  - MPE: enables `MPEInstrument` handling; per-note pitch bend replaces the channel bender for that note; per-note pressure routes to the same destinations as velocity with its own amount. When off, standard channel handling.
  - Both extras live in the modern strip, not on the panel, and are saved in the preset with a visible badge when on.
- Standalone: audio/MIDI device panel, MIDI input selection, computer-keyboard input for the performer layer.

## 5. UI

- Vector panel drawn in SVG from the photographs and the 1060 mm width (own lettering and logo, original layout, proportions and colour coding), rendered through a JUCE LookAndFeel with procedural shading for slider caps, LEDs, slide switches, the 7-segment display and the end cheeks. Bitmaps at 1x/2x/3x only where vector cannot carry a texture.
- Behaviour fidelity: sliders with the printed scales; detented HPF; three-position slide switches; latching LED buttons; momentary LFO TRIG; bank buttons labelled 1(6) 2(7) 3 4 5 with the bank 5 + 1/2 chord; display showing bank and patch, both edit dots, "- -", "_ _", "Pr", "Er"; MANUAL mode; WRITE and copy procedures as on the hardware plus a modern save dialog.
- Modern strip (separate from the panel): preset browser, A/B, undo, MIDI learn, settings (tuning reference, extras, tolerance/condition, scale), performer view.
- Resizable with fixed aspect ratio, Retina-correct, accessibility titles on every control, keyboard operation.
- Mouse mapping: relative drag by default, absolute on click as option, Shift for fine, double-click to reset, wheel step.

## 6. Phases and milestones

### Phase 0: foundations (weeks 1 to 2)
- Repo skeleton, CMake, JUCE 9 submodule, pamplejuce-style CI building AU / VST3 / Standalone universal, pluginval level 5 on every PR, Catch2 harness, clang-format, licence and NOTICE files, product name and identifiers.
- `Calibration` loader and `calibration/juno60.json` v0 with Service Notes and literature values so development is not blocked on the research track.
- Exit: empty synth passes pluginval and auval in Logic and Cubase.

### Phase 1: core voice (weeks 2 to 6)
- Master clock, DCO with integer periods, anti-log CV staircase, BLEP reset, pulse, sub, noise.
- Mixer, IR3109 model with ngspice cross-check, HPF, BA662 VCA, IR3R01 envelope, LFO.
- Six voices with rotary allocation, bender, octave transpose. Temporary generic UI.
- Unit tests: filter response vs ngspice at 20 grid points within 0.5 dB; self-oscillation at 248 Hz for the calibration patch within 2 %; key follow +2 octaves at C6 within 1 %; envelope attack 3 s at slider top within 5 %; DCO period exact; alias floor below -90 dBFS at C7.
- Exit: the 56 factory patches load from CSV and play recognisably.

### Phase 2: chorus, performance controls, presets (weeks 6 to 9)
- Chorus board model with the three modes, mute fade, noise.
- Arpeggiator, hold, key transpose, patch memory semantics, display logic.
- Preset system, JSON format, A/B, undo, factory bank, 106 .syx import.
- Golden-render tests for 12 representative patches.
- Exit: full MIDI map and preset management working in Logic and Cubase.

### Phase 3: panel UI (weeks 8 to 12, overlaps phase 2)
- SVG panel from the photographs; LookAndFeel; all controls; display; modern strip.
- Resizing, Retina, accessibility.
- Exit: side-by-side with the photographs at matching scale shows matching layout and proportions; usability pass with mouse and keyboard.

### Phase 4: calibration and fidelity (weeks 10 to 14)
- Replace v0 values with calibration v1 from the research track (simulation and published measurements, each with a source tag).
- Simulation comparison suite: for each block, render the plugin block and the ngspice reference under the same stimulus and report the error; thresholds tracked in CI as a report, not a gate.
- Tolerance layer and the condition control (filter drive, chorus noise, voice spread).
- Listening sessions on the 56 factory patches against Roland's and TAL's plugins as a sanity check (not a target); iterate on the blocks that miss.
- Exit: `docs/calibration-report.md` lists every constant with its source and every assumed value with its uncertainty, and the README's fidelity statement matches it.

### Phase 5: extras and Windows (weeks 13 to 15)
- Velocity and MPE toggles and routing; preset badge; tests that the extras-off path is bit-identical to phase 4 output.
- Windows Standalone and VST3 (x64, MSVC, `windows-latest` CI job with pluginval and a zip artifact); Windows preset folder; no AU or AAX.
- Exit: both extras work in Logic (MPE via a Seaboard-type controller or Logic's MPE test) and Cubase; the Windows VST3 passes pluginval and loads in Cubase on Windows.

### Phase 6: release engineering (weeks 15 to 18)
- **Windows installer:** Inno Setup or WiX, VST3 to `C:\Program Files\Common Files\VST3`, standalone to Program Files, code signing when a certificate exists.
- **Mac installer:** a signed and notarized `.pkg` built in CI (pkgbuild + productbuild) that installs the AU to `/Library/Audio/Plug-Ins/Components`, the VST3 to `/Library/Audio/Plug-Ins/VST3` and the standalone app to `/Applications`, with per-component choices, a welcome and licence pane, the factory preset bank, and an uninstall script. Also a plain zip and a Homebrew cask in the project's own tap. Universal binary, macOS 11 minimum.
- Apple Developer ID Application and Installer certificates ($99/yr programme), hardened runtime, timestamped signatures, notarytool submission and stapling, all from CI secrets.
- pluginval level 10 nightly, RADSan run, performance budget: six voices with chorus under 5 % of one Apple M1 core at 48 kHz / 128 samples.
- User manual, MIDI implementation chart, preset format doc, contributing guide.
- v1.0.

### Phase 7: performance layer (after v1.0, weeks 19 to 26)
Scale mode and chord mode, both toggleable, as a MIDI input transform ahead of the voice allocator, bypassable, with no change to the synth engine.

- **Grid abstraction:** `GridInput` (pad index, velocity, on/off, from a controller driver, the on-screen grid or the computer keyboard) and `GridDisplay` (set pad colour; implemented by the on-screen grid always, and by controllers that support it).
- **Scale mode:** root and scale (major, natural/harmonic/melodic minor, pentatonics, blues, dorian, mixolydian, lydian, phrygian, chromatic, user); pad-to-degree layouts (Polyend-style folded rows with a configurable row offset); octave shift; "snap incoming notes to scale" option for any MIDI keyboard.
- **Chord mode:** pad = scale degree, chord type derived diatonically (triad, 7th, sus, add9, power) with per-pad override; inversion and spread; voice budget rule for six voices (drop the 5th, no doubling, or steal) chosen in settings; strum time option; feeds the existing Juno arpeggiator so chord pad + ARPEGGIO ON is the classic move.
- **Both modes at once:** rows split between chord pads and scale pads (Polyend Play style), split point configurable.
- **Controller drivers:**
  - Circuit Rhythm (owner's unit): input only. Listens on the selected track's channel; Note View chromatic pads mapped by received note number so the Rhythm's octave buttons carry through; velocity used when the velocity extra is on. The Rhythm shows its own fixed keyboard colours; our on-screen grid shows degrees, chords and sounding notes. Documented in research/06.
  - Launchpad Mini MK3 / X / Pro MK3: input and display via programmer mode (SysEx enter, note-on velocity as palette colour, RGB SysEx), protocol in research/06 section 5. First LED-feedback target.
  - Ableton Move: not in scope (no official controller mode).
- **Computer keyboard (standalone):** two rows mapped to pads, key-repeat suppressed, fixed velocity, 6-key rollover documented; in a DAW the transform acts on the host's notes instead.
- **LED feedback semantics:** root pads one colour, other scale degrees a second colour, chord pads a third, sounding pads bright, out-of-scale pads off; same palette on screen and on the Launchpad.
- Exit: scale and chord mode usable from the Mac keyboard in the standalone app and from the Circuit Rhythm, with the on-screen grid; Launchpad LED feedback verified on at least one Launchpad model; the transform off path is bit-identical to v1.0.

## 7. Testing strategy

- Unit: every DSP block against its calibration table and its ngspice reference.
- Property: block-size independence (1, 17, 64, 512 samples give identical output), sample-rate independence (44.1 to 192 kHz within tolerance), denormal-free silence, no allocation on the audio thread (RADSan).
- Golden renders: fixed MIDI, fixed sample rate, committed WAVs, tolerance on RMS and spectral difference, regenerated deliberately with a changelog note.
- Host: pluginval 5 on PRs, 10 nightly; auval strict; manual smoke list for Logic and Cubase (automation, preset recall, state round-trip, PC, CC, extras).
- Perceptual: blind A/B sessions on the 56 factory patches against the commercial references, recorded as pass/fail with notes, understood as a sanity check rather than a fidelity proof.

## 8. Risks

| Risk | Mitigation |
|---|---|
| No hardware: some constants are simulation- or literature-derived | every constant carries a source tag; the README states the method; the bench protocol and a drop-in calibration file are ready if a unit appears |
| IR3109 drive level and Q compensation off by a few dB changes the character | ngspice cross-check against the Service Notes anchors (248 Hz, 4 Vp-p, 30 kHz, 40 Hz to 5 kHz); condition control for drive |
| Chorus pre-filter corner disagreement (6.5 vs 9.7 kHz) | settled by simulating the schematic values |
| Trade dress complaint | original name, lettering, logo and textures; layout only |
| Performance layer competes with fidelity work | phase 7 opens only after v1.0 |
| Circuit Rhythm cannot show the scale on its pads | on-screen grid always; Launchpad as the lit-grid target |

# Development plan

## 1. Stack

| Concern | Choice | Why |
|---|---|---|
| Framework | JUCE 9, used under AGPLv3 | only mature option with AU + VST3 + AAX + standalone; AAX SDK bundled under GPLv3 terms |
| Project licence | GPLv3 | compatible with JUCE AGPL, the AAX SDK's GPL option and the GPL DSP libraries worth reusing |
| Build | CMake 3.25+, pamplejuce layout, Ninja | CI-friendly, universal binaries |
| Language | C++20 | |
| Tests | Catch2 v3, pluginval, auval, golden-render tests | |
| Extra formats | CLAP via clap-juce-extensions (if JUCE 9 compatible) | free, used by Bitwig and REAPER users |
| Optional libs | chowdsp_wdf (BSD-3), HIIR (WTFPL), chowdsp_utils BSD modules; GPL chowdsp DSP modules and jpcima BBD code where they save time | licences compatible |
| Fitting and offline sim | Python 3 (numpy, scipy, soundfile, matplotlib), ngspice or ACME.jl | |
| CI | GitHub Actions macos-15 (arm64) building universal; macos-15-intel for Intel-side tests | free for public repos |

Naming: an original product name, vendor name and 4-character codes with no Roland or Juno trademark. Decide before the first tagged release; the codebase uses "jnsynth" internally.

## 2. Architecture

```
jnsynth/
  CMakeLists.txt
  cmake/                 toolchain, signing, pluginval helpers
  libs/                  JUCE and third-party as submodules
  calibration/           juno60.json, juno106.json (fitted constants; see research plan)
  src/
    dsp/                 framework-free, header-only where sensible, testable without JUCE
      core/              VoiceClock (master clock + 8253 periods), tables, oversampling
      dco/               Dco (integrator, reset, DAC staircase), Waveshaper (saw, pulse, sub), Noise
      mixer/             Mixer (resistive sum, taper, drive scaling)
      vcf/               Ir3109 (4-stage nonlinear ZDF OTA cascade + BA662 feedback, Q comp), Hpf
      vca/               Ba662Vca (ENV/GATE, offset thump), LevelVca
      env/               Ir3r01Envelope (60), FirmwareEnvelope (106)
      lfo/               Lfo (triangle, delay hold + fade, trigger modes)
      chorus/            BbdLine (Holters-Parker variable rate), ChorusBoard (filters, LFO, summers, mute)
      voice/             Voice (one of six), VoiceAllocator (rotary / non-rotary / mono / 106 modes)
      perf/              Arpeggiator, Hold, KeyTranspose, Bender
      Synth.h            top level: 6 voices -> sum -> HPF -> level VCA -> chorus -> volume
      Tolerances.h       per-voice "condition" offsets
    plugin/              JUCE AudioProcessor, parameter layout, state, MIDI handling, SysEx
    presets/             preset model, JSON format, factory bank, A/B, undo, 106 .syx import/export
    midi/                CC map, NRPN, MIDI learn, program change, UMP
    ui/                  panel renderer, controls, LookAndFeel, display, preset browser, settings
    standalone/          audio and MIDI device settings extras
  tests/
    unit/                per-block Catch2 tests against calibration tables and ngspice references
    golden/              offline renders of fixed MIDI vs committed WAVs
    hardware/            null/spectral comparisons vs bench captures (manual, not in CI)
  tools/
    fit/                 Python fitting scripts (research plan section 3)
    sim/                 ngspice / ACME netlists
    panel/               SVG panel source and asset export
  docs/
```

Design rules:

- `src/dsp` has no JUCE dependency so blocks can be unit-tested, fuzzed and compared to ngspice in milliseconds.
- Every constant that describes the hardware lives in `calibration/*.json` and is loaded into a `Calibration` struct at startup; tests and DSP share the same file. No magic numbers in DSP code.
- Per-voice tolerance is a separate layer (`Tolerances`) applied on top of the calibrated nominal model so "factory fresh" is a single switch.
- Internal sample rate: host rate, with 2x oversampling around the IR3109 and the BBD line; the DCO renders band-limited directly (PolyBLEP/BLEP with the integer period and finite-slope reset), so no global oversampling.
- Control-rate signals (envelopes, LFO, CVs) are computed per sample for the filter and VCA because the hardware is continuous there; the 106 mode quantises them to the firmware tick on purpose.
- Parameter IDs are strings fixed at v1 and never reordered (AAX identifies by index).

## 3. DSP block specifications

Each block gets a one-page spec in `docs/spec/circuit/` before coding. Summary of the modelling approach per block:

### DCO
- One master clock shared by all voices; bend, LFO and tune modulate its frequency in cents exactly like the varicap. Per voice: integer period N from the 8253 (rounding rule from measurement), so pitch quantisation at the top of the range is reproduced.
- Ramp: falling integrator with amplitude from the DCO-CV staircase table (per note, per RANGE); reset via a finite-slope flyback of measured duration; optional incomplete-discharge offset. Anti-aliasing: BLEP at the reset edge, BLAMP for the slope change at the flyback.
- Pulse: comparator against PWM CV with the raised-cosine duty law 50 to 95 %, no level compensation, BLEP edges. Sources MAN / LFO / ENV.
- Sub: divide-by-two square phase-locked to the reset, BLEP edges.
- Noise: one shared generator with the measured spectrum (low-passed plus any supply component), identical signal to all voices.

### Mixer
Resistive sum with measured relative levels and slider tapers; output scaled to the measured drive into the IR3109 input attenuator.

### VCF (IR3109 + BA662)
- Four TPT one-pole OTA-C stages with tanh in each stage (Vt 26 mV scaled to the real drive), global inverting feedback through a BA662 model with input-side Q compensation at the measured coefficient, Newton iteration (2 to 3 steps) at 2x oversampling. Validated against an ngspice OTA-macro-model of the same netlist.
- Cutoff CV summer with measured slider taper, ENV depth in octaves and polarity, LFO depth, key follow with C4 pivot, tempco omitted (room temperature) unless measurement shows otherwise.
- Resonance law: self-oscillation onset and amplitude from the measured self-oscillation sweep; passband loss vs resonance falls out of the circuit model, checked against measurement.
- Per-voice cutoff and resonance offsets from the six-voice spread.

### HPF
One-pole RC with the four measured states applied to the voice sum; 106 mode uses the shelf table.

### VCA
BA662 gain linear in control current; ENV or GATE control with the measured 3 ms / 6 ms gate edges; residual offset thump as a small CV-shaped DC term per voice; shared LEVEL VCA with measured taper, before the chorus.

### Envelope
- Juno-60: IR3R01 model with stepped-oscillator timing, attack as truncated exponential toward an overshoot target, decay and release exponential with duration independent of sustain, slider-to-time tables from measurement, sustain law from measurement, retrigger from current level, gate/trigger tied.
- Juno-106: integer accumulator at the firmware tick rate with linear-ish attack and stepping at slow settings, from the ROM analysis.

### LFO
Triangle with measured rate table; delay as hold then fade of depth; AUTO / MAN trigger semantics from the behaviour spec; phase reset behaviour from measurement. Destinations: master clock (pitch), per-voice cutoff summer, PWM.

### Chorus
- Input gain stage, measured pre-filter (two Sallen-Key pairs plus the real pole), two BBD lines per Holters and Parker (variable clock, 256 stages, transfer loss vs clock, saturation, measured noise floor and clock residue), post filters, JFET mute with the measured fade, summers at 47 k / 39 k into 10 k. Right channel LFO inverted for I and II; I+II in phase.
- Mode table: 0.513 / 0.863 / 9.75 Hz, delay 1.66 to 5.35 ms and 3.3 to 3.7 ms, refined by measurement.
- 2x oversampled line for the BBD sampling (or BLEP on the sample-and-hold steps per DAFx-25 if CPU matters).

### Voice allocation and performance
Rotary assignment with oldest-voice stealing (60 default), non-rotary and mono as hidden modes, 106 Poly 1 / Poly 2 / Unison; HOLD (last six keys); arpeggiator UP / U&D / DOWN, 1 to 3 octaves, 1.5 to 50 Hz, external clock replaced by host sync option; key transpose; bender with DCO and VCF depths; octave transpose.

## 4. Plugin layer

- Parameters: one `AudioProcessorValueTreeState` parameter per panel control, discrete controls as choices, switch states as parameters, LEDs driven from parameters.
- State: APVTS XML + preset name + A/B slots + MIDI map + UI size + tolerance settings, versioned.
- Presets: JSON format with schema version; factory bank in BinaryData; user folder under ~/Library/Audio/Presets; browser with banks, search, tags, favourites; A/B with copy; undo/redo; "modified" indicator; Juno-106 .syx import and export; Juno-60 patch-sheet CSV import for the 56 factory patches.
- MIDI: see research/04 section 7. Default CC map published as a table and json; MIDI learn; NRPN; 14-bit CC; program change and bank select; SysEx 0x30 / 0x31 / 0x32 both directions on a selectable channel; UMP accepted; MPE off by default; optional velocity routing.
- Standalone: audio/MIDI device panel, MIDI input selection, panic, activity LED.

## 5. UI

- Vector panel drawn once in SVG (own lettering and logo, original layout and proportions from the photographs and measurements), rendered through a JUCE LookAndFeel with procedural shading for slider caps, LEDs, slide switches, the 7-segment display and the wooden end cheeks. Textures as 1x/2x/3x bitmaps only where vector cannot carry them.
- Behaviour fidelity: sliders with the printed scales, detented HPF, three-position slide switches, latching LED buttons, momentary LFO TRIG, bank 5 + 1/2 for banks 6/7, display showing bank/patch, edited dot, "--", "__", "Er"; MANUAL mode; WRITE procedure as on the hardware plus a modern save dialog.
- Modern additions in a separate strip below or above the panel (so the panel stays original): preset browser, A/B, undo, MIDI learn mode, settings, tolerance/condition, 60/106 mode, scale.
- Resizable with fixed aspect ratio, Retina-correct, accessibility titles on every control, keyboard operation.
- Mouse mapping: relative drag by default, absolute on click as option, Shift for fine, double-click to reset, wheel step.

## 6. Phases and milestones

### Phase 0: foundations (weeks 1 to 2)
- Repo skeleton, CMake, JUCE 9 submodule, pamplejuce-style CI building AU / VST3 / AAX / Standalone universal, pluginval level 5 on every PR, Catch2 harness, clang-format, licence and NOTICE files.
- `Calibration` loader and the first `calibration/juno60.json` filled with literature values (research/01 and 02) so development is not blocked on the bench.
- Exit: empty synth passes pluginval and auval in Logic, Cubase and Pro Tools Developer.

### Phase 1: core voice (weeks 2 to 6)
- DCO with master clock, integer periods, DAC staircase, BLEP reset, pulse, sub, noise.
- Mixer, IR3109 model with ngspice cross-check, HPF, BA662 VCA, IR3R01 envelope, LFO.
- Six voices with rotary allocation, bender, octave transpose. Minimal temporary UI (generic sliders).
- Unit tests: filter response vs ngspice at 20 grid points within 0.5 dB; self-oscillation frequency vs calibration anchor within 2 %; envelope times within 5 % of tables; DCO period exact; alias floor below -90 dBFS at C7.
- Exit: the 56 factory patches load from CSV and play recognisably.

### Phase 2: chorus, performance, presets (weeks 6 to 9)
- Chorus board model with the three modes, mute fade, noise.
- Arpeggiator, hold, key transpose, patch memory semantics, display logic.
- Preset system, JSON format, A/B, undo, factory bank, 106 .syx import.
- Golden-render tests committed for 12 representative patches.
- Exit: full MIDI map and preset management working in all three hosts.

### Phase 3: panel UI (weeks 8 to 12, overlaps phase 2)
- SVG panel from photographs and measurements; LookAndFeel; all controls; display; modern strip.
- Resizing, Retina, accessibility.
- Exit: a side-by-side with the photographs at 100 % shows matching layout and proportions; usability pass with a mouse and with a keyboard.

### Phase 4: calibration and fidelity (after the bench session, weeks 10 to 14)
- Replace literature values with `calibration/juno60.json` v1 from the fitting tools.
- Hardware comparison suite: for each capture in the index, render the same patch and note in the plugin and report spectral envelope distance, level error and envelope timing error; set acceptance thresholds and track them in CI as a report (not a gate).
- Per-voice tolerance layer and the condition control.
- Listening sessions against the hardware with the owner; iterate on the blocks that miss.
- Exit: acceptance table in `docs/calibration-report.md` with every block within its threshold, and a documented list of known deviations.

### Phase 5: Juno-106 mode (weeks 14 to 18, needs a 106 and its calibration file)
- Firmware envelope, LFO quantisation, portamento, Poly 1 / Poly 2 / Unison, HPF shelf table, LFO range, 128 patches, SysEx in and out, panel variant (or a 106 skin).

### Phase 6: release engineering (weeks 16 to 20)
- Apple Developer ID signing, notarization, .pkg installer, Homebrew tap, GitHub Releases.
- Avid developer registration, PACE signing from the maintainer machine, AAX smoke test in release Pro Tools.
- pluginval level 10 nightly, RADSan run, performance budget: six voices with chorus under 5 % of one Apple M1 core at 48 kHz / 128 samples.
- User manual, MIDI implementation chart, preset format doc, contributing guide.
- v1.0.

## 7. Testing strategy

- Unit: every DSP block against its calibration table and its ngspice reference.
- Property: block-size independence (1, 17, 64, 512 samples give identical output), sample-rate independence (44.1 to 192 kHz within tolerance), denormal-free silence, no allocation on the audio thread (RADSan).
- Golden renders: fixed MIDI, fixed sample rate, committed WAVs, tolerance on RMS and spectral difference, regenerate deliberately with a changelog note.
- Hardware comparison: offline, report-only, per capture.
- Host: pluginval 5 on PRs, 10 nightly; auval strict; manual smoke list for Logic, Cubase, Pro Tools (automation, preset recall, state round-trip, PC, CC, SysEx).
- Perceptual: blind A/B sessions with the owner on the 56 factory patches, recorded as pass/fail with notes.

## 8. Risks

| Risk | Mitigation |
|---|---|
| No bench access to a calibrated Juno-60 | ship on literature values and say so; keep calibration as a drop-in file |
| IR3109 drive level and Q compensation wrong by a few dB changes the character | ngspice cross-check plus the self-oscillation and passband-loss measurements pin it |
| BBD model CPU cost | 2x line with Holters-Parker filters is cheap; BLEP fallback |
| AAX signing | maintainer-signed release path; AU and VST3 unaffected |
| Trade dress complaint | original name, lettering, logo and textures; layout only |
| Scope creep into 106 before the 60 is right | 106 is a separate phase gated on the 60 acceptance table |
| Behaviour spec ambiguities (LFO trigger, key transpose direction, voice stealing detail) | resolved from the PDFs and bench in research week 1 to 4; until then implement the most-cited behaviour and flag it in code |

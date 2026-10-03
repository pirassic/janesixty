# Research plan

Purpose: close every gap listed in research/01 section 12 and research/02 section 9 with primary documents and bench measurements, and produce the calibration data the DSP model is fitted to.

Deliverables of the research track:

- `docs/spec/behaviour.md`: frozen behaviour specification, every control and mode, with manual page references.
- `docs/spec/circuit/`: per-block schematic extracts (redrawn, own drawings), component tables, derived transfer functions.
- `measurements/`: raw captures (WAV, 24-bit, 192 kHz) plus a CSV index; not committed to git, stored in a release asset or external bucket, with checksums committed.
- `calibration/*.json`: fitted model parameters per block, versioned, consumed by the DSP code and the test suite.
- `docs/research/`: the running notes (already started).

## 1. Document track (no hardware needed)

### 1.1 Read the primary PDFs first-hand
Owner's manual and Service Notes (Juno-60), plus the Juno-106 owner's manual and Service Notes for phase 2. Work the checklist in research/02 section 9 and fill in research/01 section 12 items that are on paper:

- per-voice DCO board: integrator resistor set per RANGE, mixer leg resistors, comparator network, S/H or DAC arrangement for the DCO CV;
- IR3R01 pinout, reference voltage, timing network, VR6;
- chorus mode-switch resistor values for I / II / I+II and the LFO board;
- HPF position 0 wiring;
- adjustment summary (all trimmers, targets, test points);
- the block diagram and the key-assign description in the Service Notes;
- panel legends, exact text, symbols, slider scales.

### 1.2 Anwander's pages and Electric Druid
Read florian-anwander.de (Juno-6 DCO, waveshaper, VCF, chorus) and the Electric Druid IR3109 and Juno DCO pages for the hand-drawn circuit explanations; cross-check against the Service Notes.

### 1.3 Firmware behaviour
- Juno-60 CPU: no public disassembly found. Derive timing (scan period, note latency, 8253 mode, rounding) from bench measurement (section 2.8).
- Juno-106: use the j106roms disassembly for envelope tick rate, LFO update rate, portamento interpolation and assigner rules.

### 1.4 Reference simulations
Build offline circuit simulations to validate the real-time models before the bench session and to fill gaps where measurement is impractical:

- ngspice or ACME.jl netlists for: one IR3109 stage pair with BA662 feedback (OTA macro-models), the HPF, the chorus input and output filter chains, the chorus clock VCO, the DCO integrator with reset transistor.
- Outputs: frequency responses, step responses, harmonic series vs drive level, to compare against the DSP unit tests.

## 2. Bench track (needs a real Juno-60)

### 2.1 Preparation
- Service the unit: PSU rails, then the full Service Notes adjustment procedure (master clock 442 Hz, bender, LFO rate, LFO offset, delay time, ENV TIME 3 s, per-voice VCA offset, per-voice VCF 248 Hz anchor and resonance level). Record every trimmer target reached. This makes the measurements describe the design, not one unit's drift.
- Warm-up 30 minutes before each session; log room temperature.
- Note the CPU ROM variant (uPD8049C-238 or -380).
- Decide tuning: measure at the Service Notes 442 Hz, record the deviation; the plugin defaults to 440.

### 2.2 Signal paths and gear
- Audio: 24-bit / 192 kHz interface, balanced line inputs, output level switch at H (0 dBm), VOLUME at a marked, repeatable position. Record L and R separately. Record a 1 kHz calibration tone from a known source through the same input chain for absolute level.
- CVs: DC-coupled card or oscilloscope with data export for envelope outputs, DCO CV, LFO, chorus LFO.
- Oscilloscope 50 MHz or better plus frequency counter for the master clock, 8253 outputs, BBD clocks.
- Note automation: a DCB adaptor (Kenton Pro DCB MK3 or Valpower) driven from a scripted MIDI sequence, so every capture has identical timing. Do not use a Juno-66 or other CPU replacement on the reference unit.
- Panel automation: not possible on the 60. Use a slider jig (printed scale at 0, 1, ... 10) and store each grid state as a patch so it can be re-recalled; log every panel state in the capture index.

### 2.3 DCO captures
Direct voice-board test-point probes where available, otherwise VCF fully open, resonance 0, HPF 0, chorus off, VCA gate.
- Saw alone, pulse alone (PWM MAN at 0, 2.5, 5, 7.5, 10), sub alone, noise alone, saw + pulse, at every C and every F from C2 to C7 in each RANGE. Two seconds each.
- Deliverables: exact period per note (recovers N and the master clock), amplitude vs note per RANGE (the DAC staircase), reset flyback shape and duration at 192 kHz plus scope capture, saw curvature, DC offset, PWM duty vs slider, pulse level vs duty, sub phase relative to saw, noise spectrum and any supply-related components.
- LFO to pitch at depth 2.5, 5, 7.5, 10 with RATE at 2.5; bender full left and right with DCO depth at 5 and 10. Confirm +-700 and +-300 cents and the slider laws.

### 2.4 Mixer and drive into the filter
- Saw + pulse + sub (10) + noise (10) versus each alone, filter open: relative levels into the VCF and the onset of OTA compression.
- Sub slider and noise slider at 0, 2.5, 5, 7.5, 10 for the taper.

### 2.5 VCF captures
Source: DCO saw at C3 as a harmonic comb, plus a second pass with sub only as a near-clean square. Sustained 4 s notes.
- Grid: FREQ 0 to 10 in 0.5 steps x RES 0, 2.5, 5, 7.5, 10 x KYBD 0 and 10, at C2, C4 and C6. At KYBD 10 confirm the C4 pivot and the octave per octave slope.
- Self-oscillation: RES 10, all waveforms off, FREQ 0 to 10 in 0.5 steps: frequency and amplitude per voice (recovers the exponential converter law, the 248 Hz anchor, the slider taper and the per-voice offsets).
- ENV to cutoff: FREQ 0, ENV 2.5, 5, 7.5, 10, both polarities, A 0, D 10, S 10, so the sweep extent in octaves can be read.
- LFO to cutoff: FREQ 5, LFO depth 2.5, 5, 7.5, 10, RATE 2.5.
- Passband loss vs resonance at FREQ 7.
- Six-voice spread: repeat the self-oscillation sweep once per voice (cycle voices with six successive notes).

### 2.6 VCA and envelope captures
- ENV mode, saw, filter open: for A, D, R separately at 0, 1, 2, ... 10 (11 points each), S at 0 and 5 for decay; S at 0, 2.5, 5, 7.5, 10 for the sustain law. Gate from DCB with fixed 4 s hold. Export envelope CV from the scope in parallel for curve shape.
- GATE mode edges at 192 kHz.
- VCA offset thump: silent voice (all waveforms off, RES 0) with a fast envelope, per voice.
- Retrigger: new note during attack and during release on the same voice (force with mono test mode).
- LEVEL slider taper: -5 to +5 in 1 steps at fixed patch.

### 2.7 HPF, LFO, chorus
- HPF: noise at 10 through positions 0 to 3; confirm 154 / 339 / 720 Hz and whether 0 is flat.
- LFO: RATE 0 to 10 in 1 steps (frequency, triangle symmetry); DELAY 0 to 10 in 1 steps (hold and fade times); AUTO vs MAN trigger behaviour including whether phase resets; LFO TRIG button with PWM-by-LFO active.
- Chorus, each of I, II, I+II, L and R recorded separately: 1 kHz sine from the DCO (self-oscillating filter) for LFO shape, rate and delay range; saw for the wet spectrum; silence for noise floor and clock residue (192 kHz; look for the clock and its sidebands); a level series (LEVEL -5 to +5) for saturation; chorus on/off switching for the mute fade. Scope the BBD clock pins directly to confirm the 24 to 77 kHz range and the clock-vs-LFO law.
- Output stage: absolute output level vs VOLUME at the three level switch positions; output impedance.

### 2.8 Firmware timing
- Note-on latency from DCB byte to first ramp reset, 20 trials.
- Key scan period: scope the keyboard matrix strobe.
- Arpeggio: gate fraction and timing at RATE 2.5, 5, 10; up/down turnaround behaviour; HOLD interaction.
- Voice stealing: scripted 7- and 8-note chords, releasing voices in various orders, record which voice is reused.
- Test mode (power on with KEY TRANSPOSE held): confirm mono, rotary, non-rotary modes.

### 2.9 Capture index
Every file is named `<block>_<param-state>_<note>_<take>.wav` and listed in `measurements/index.csv` with panel state, patch slot used, temperature, gear chain and operator notes.

## 3. Fitting track (turns captures into calibration files)

Python (numpy, scipy, soundfile) in `tools/fit/`:

- DCO: period detection, amplitude-vs-note table per RANGE, flyback duration, pulse duty curve.
- VCF: linear response per grid point by least squares against the comb; fit expo converter (Hz per slider unit, per octave), feedback gain vs RES, Q-compensation coefficient, tanh drive level, per-voice offsets. Validate against the ngspice model.
- Envelope: segment time vs slider fits, curve shape, sustain law.
- LFO: rate and delay tables.
- Chorus: LFO rate and shape (modulation extraction), delay range, filter pole fits, wet gain, noise spectrum, saturation curve.
- Output: `calibration/juno60.json` with every table and constant, plus a report with plots in `docs/calibration-report.md`.

## 4. Juno-106 phase (needs a real 106)

Same protocol with these additions: SysEx 0x32 automation of every parameter (so the full grid can be scripted), the Poly 1 / Poly 2 / Unison assigner, portamento stepping, envelope stepping at slow rates, LFO quantisation, HPF bass boost, chorus I/II and whether I+II exists, 80017A-equipped vs replacement-module voice differences, MIDI function switch modes.

## 5. Source inputs needed from the owner

Hard requirements for a faithful model:

1. **A Juno-60**, serviced and calibrated per Service Notes, available for two or three bench days. Ideally a second unit later to see unit-to-unit spread.
2. **Primary documents**, read by a person: Juno-60 owner's manual and Service Notes (supplied URLs), Juno-106 owner's manual and Service Notes for phase 2. Keep them out of the public repo.
3. **Test gear**: 192 kHz audio interface; oscilloscope with export; frequency counter (or scope with a counter); DCB adaptor plus MIDI interface; DC-coupled capture for CVs (or scope export); multimeter.
4. **Recordings** per sections 2.3 to 2.8, with the capture index. If a bench session is not possible, the fallback is the pendragon-andyh/Juno60 measurements plus the Roland plugin as a secondary reference, and the model's claim to fidelity is correspondingly weaker. State this honestly in the README.

For the UI:

5. **Photographs**: straight-on, evenly lit, high-resolution images of the full panel, the bender panel, the rear panel, the keyboard end cheeks, and close-ups of one slider cap, one LED button, one slide switch, the display, and the panel lettering, with a ruler in frame. Also a photo with power on and several LEDs lit.
6. **Panel dimensions**: overall panel width and height, slider travel length, slider spacing within a section, button pitch, display digit size.

Nice to have:

7. Scans of the factory patch sheets and the blank patch chart.
8. Any existing recordings of the unit playing the factory patches (used only as a perceptual check, never as samples).

## 6. Timeline

| Week | Work |
|---|---|
| 1 | Read primary PDFs, freeze behaviour spec, redraw block schematics, set up ngspice/ACME models |
| 2 | Build the fitting tools against the pendragon dataset so they are ready before the bench session |
| 3 to 4 | Bench session(s) on the Juno-60; capture index complete |
| 5 | Fitting, calibration file v1, calibration report |
| later | Juno-106 bench session and calibration file |

The development track (02) starts in week 1 in parallel; it consumes calibration v1 in week 5 and runs on literature values before that.

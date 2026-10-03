# Juno-60 vs Juno-106, prior art, modelling literature, measurement method

Status: research notes, compiled 2026-10-03. Many primary hosts were blocked from the sandbox; items marked **[unverified]** rest on search snippets or secondary sources.

## 1. Juno-60 vs Juno-106

### 1.1 Digital control

| | Juno-60 | Juno-106 |
|---|---|---|
| CPU | NEC uPD8049C | two NEC uPD7810/7811 (assigner + voice CPU) |
| Master clock | analogue LC VCO ~1 to 3.5 MHz; bend and LFO modulate the clock | 8 MHz crystal; bend, LFO, portamento summed in software into the timer value |
| Pitch counters | Intel 8253 (two chips) | same |

On the 60, vibrato and bend are continuous and common to all voices. On the 106 they are quantised by the CPU update rate (audible stepping on slow LFO or portamento is reported). Update rate [unverified].

### 1.2 DCO
Same principle (ramp charged from a CPU-generated compensation CV, reset by the 8253; saw, pulse comparator, sub from a divide-by-two flip-flop, one shared noise source). Forum consensus: DCO, VCF and VCA circuits are essentially identical; differences are packaging (discrete IR3109 + 2 x BA662 + film caps on the 60 vs the 80017A hybrid with SMD ceramic caps on the 106) and the digital control above. PWM source: 60 has LFO / MAN / ENV, 106 only LFO / MAN. 106 range and wave switches are LED push-buttons.

### 1.3 Voice module
60: per voice one IR3109, one BA662 (resonance), one BA662 (VCA), through-hole parts. 106: Roland 80017A hybrid (SMD IR3109N + 2 x BA662F in resin). The resin becomes conductive with age; replacement modules (AR80017A, Rosen Sound, Borish) use LM13700 + buffers and reportedly sound slightly different.

### 1.4 Filter and HPF
Both are the IR3109 4-pole cascade with BA662 feedback, self-oscillating. Reported: 60 "squelchier", 106 "cleaner, slightly more top and bottom". Hypothesised causes: cap types, VCA loading of passive stages, HPF stage, CV scaling. "106 cutoff range differs" [unverified].

HPF is a single global passive RC after the voice sum, selected by a 1-of-4 analogue demux.
- 60: 0 = flat, 1 to 3 progressively higher corners (154 / 339 / 720 Hz from component values).
- 106: 0 = **bass boost** (low shelf), 1 = flat, 2 and 3 = HPF. KR-106 models it as +10 dB shelf below 150 Hz / flat / 240 Hz / 720 Hz (author's fit).

### 1.5 VCA
Same BA662 topology; inside the 80017A on the 106.

### 1.6 Envelopes (major difference)
60: hardware IR3R01 per voice, true RC-type curves, very fast attack. 106: software envelopes from the voice CPU through a multiplexed DAC, linear-ish attack, stepped at slow rates, slightly slower minimum times. 106 spec: A 1.5 ms to 3 s, D 1.5 ms to 12 s, R 1.5 ms to 12 s.

### 1.7 LFO
Both have Rate and Delay. 60: 0.3 to 20 Hz, delay 0 to 2 s, LFO TRIG button with AUTO/MAN. 106: 0.1 to 30 Hz, delay 0 to 3 s, automatic trigger only, bender lever pushed forward triggers LFO with an "LFO Trig Sens" knob.

### 1.8 Chorus
Both: two MN3009 + two MN3101, one triangle LFO, right channel inverted, ~12 dB/oct pre-filter, no compander (hence the hiss). 60: I 0.513 Hz, II 0.863 Hz, I+II 9.75 Hz. 106: I ~0.5 Hz, II ~0.8 Hz; existence of a distinct I+II on 106 hardware is [disputed]. Noise ranking reported 106 > 60 > 6; causes: aged MN3009s, leaky mute transistors, supply filtering. Roland's own 106 plugin reproduces the hiss.

### 1.9 Memory, performance, panel
- Patches: 60 has 56 (7 x 8); 106 has 128 (A/B x 8 x 8).
- 60 only: arpeggiator, DCB, LFO TRIG button with AUTO/MAN, ENV PWM, tape interface.
- 106 only: MIDI In/Out/Thru, polyphonic portamento (Time slider), key modes Poly 1 / Poly 2 / Unison (both pressed), bender forward push for LFO, HPF slider with bass boost, LFO to 30 Hz, patch/bank buttons under the sliders, LED push-buttons.

### 1.10 Voice allocation
- 60: cyclic round-robin, 7th key steals the oldest.
- 106 Poly 1: round-robin (portamento glides from whatever that voice last held). Poly 2: reuses the same voice while playing legato, adds voices only when more keys are held (designed for portamento). Unison: all six voices on one key, no detune control. 106 test mode: power on holding KEY TRANSPOSE; Poly1+Poly2 cycles voices with LED display, useful to isolate one voice for measurement.

### 1.11 Juno-106 MIDI
- No velocity or aftertouch. Notes, hold, bender, bender-to-LFO, program change, SysEx for every parameter and single-patch dumps. No bulk dump.
- Rear MIDI Function switch: I = notes and hold; II = plus bender, modulation, program change; III = plus SysEx. (I/II split [unverified].)
- SysEx, Roland ID 0x41:
  - Patch dump: `F0 41 30 0n pp [16 slider bytes] [sw1] [sw2] F7`, n = channel - 1, pp = patch 0 to 127. Slider order: LFO rate, LFO delay, DCO LFO, DCO PWM, Noise, VCF Freq, VCF Res, VCF Env, VCF LFO, VCF Kybd, VCA Level, A, D, S, R, Sub.
  - Manual mode message type 0x31 [format unverified].
  - Parameter change: `F0 41 32 0n cc vv F7`, cc 0x00 to 0x0F in the slider order above, 0x10 Switches 1, 0x11 Switches 2.
  - Switches 1 bits: b0 16', b1 8', b2 4', b3 pulse, b4 saw, b5 chorus off, b6 chorus level (0 = II, 1 = I). Switches 2: b0 PWM source (0 LFO / 1 MAN), b1 VCF env polarity (0 + / 1 -), b2 VCA mode (0 env / 1 gate), b3 to b4 HPF (00 = 3, 01 = 2, 10 = 1, 11 = 0). Exact bit layout [unverified against primary].
  - References: jamrouter doc/juno-106.txt, hinzen.de howto, hyperreal sysex txt, konsumer/junosex, Roland JUNO-106 to SYSTEM-8 parameter table.
- Juno-60 DCB: 14-pin, 31250 baud, 8 data bits, odd parity, 2 stop bits, note on/off only on the 60.

### 1.12 Firmware references
ErroneousBosh/j106roms (partial 106 ROM disassembly); nydell.se j106 project.

## 2. Prior art

### 2.1 Commercial

| Product | Target | Stated approach |
|---|---|---|
| Roland Cloud JUNO-60 / JUNO-106, JU-06A | 60, 106 | ACB circuit modelling; v2 adds Circuit Mod and Condition (ageing, voice drift); 60 plugin has 106 HPF voicing switch and secret I+II |
| TAL-U-NO-LX | 60 | zero-feedback-delay filter, "calibrated after a real Juno-60"; often judged closest to the 60 |
| Softube Model 84 | 106 | component-level modelling from a serviced 1984 unit; often judged closest to the 106 |
| Arturia Jun-6 V | 6 | TAE component modelling (marketing level) |
| Cherry Audio DCO-106 | 106 | no technical disclosure; good MIDI/SysEx control doc |
| u-he Diva | Juno modules | real-time circuit simulation + ZDF, per-voice tolerance controls |
| IK Syntronik Juno-60 | 60 | sample based, not a VA reference |

### 2.2 Open-source Juno projects

| Project | Licence | Language | Reusable |
|---|---|---|---|
| kayrockscreenprinting/ultramaster_kr106 | GPL-3.0 | C++/JUCE | most complete OSS Juno: TPT 4-stage OTA cascade with tanh Pade + Newton, 2x HIIR oversampling, PolyBLEP DCO with ramp curvature and reset undershoot, dual-mode ADSR, MN3009 chorus, 106 HPF, per-voice tolerances, full 106 SysEx; docs/DSP_ARCHITECTURE.md is the best written reference |
| pendragon-andyh/Juno60 | MIT | docs + JS | measurement-based analysis of a real Juno-60 (DCO, envelope, VCF/HPF, chorus, LFO, unison) |
| pendragon-andyh/junox | GPL-3.0 | JS | PolyBLEP DCO, ladder, envelope and chorus visualisers |
| jpcima/Hera | GPL-3.0 | C++/JUCE | Juno-60 synth with MPE; chorus DSP widely reused; author notes VCF/env inaccuracies |
| schollz/juno-60 | GPL-3.0 | C++ SuperCollider | port of junox |
| peterall/junologue-chorus | MIT | C++ | Juno-60 chorus I/II/I+II, small, permissive |
| trudslev/chorus-60-audio-plugin | MIT | C++/JUCE 8 | BBD chorus with clock drift, saturation, noise floor |
| gligli/juno-chorus-clone | check | KiCad | exact Juno-60 chorus board BOM |
| curlcomplex/Faust-expr (juno-106 v2) | permissive | Faust | nonlinear IR3109-style VCF, mixer loading hypothesis |
| ErroneousBosh/j106roms | n/a | asm | 106 firmware behaviour |
| polykit/dco, gerb-ster/Vulcan-DCO | check | Arduino/Pico | hardware Juno-style DCOs, ramp/reset timing |

### 2.3 Building blocks

| Project | Licence | Notes |
|---|---|---|
| sst-filters (Surge XT) | GPL-3.0 | header-only C++/SSE ladders (Huovilainen, OB-Xd, K35, diode) |
| jatinchowdhury18/BBDDelay | BSD-3 | BBD per Raffel-Smith + Holters-Parker |
| chowdsp_wdf | BSD-3 | header-only WDF library |
| chowdsp_utils | BSD-3 core / GPL-3 DSP modules | filters, ADAA waveshapers, sources, presets, plugin state |
| jpcima/bbd-delay-experimental | BSL-1.0 | Holters-Parker variable-rate BBD, directly reusable |
| jpcima/ensemble-chorus, string-machine | BSL-1.0 | Raffel-Smith BBD, Solina chorus |
| ACME.jl (Holters) | MIT | Julia nodal state-space from netlists; ideal for offline reference simulation |
| joaorossi/dkmethod | GPL-3.0 | JUCE module for nodal DK |
| Faust wdmodels, vaeffects | per lib | WDF components, TPT ladders |
| HIIR (Laurent de Soras) | WTFPL | polyphase half-band oversampling |
| NablAFx | check | differentiable grey/black-box framework (PyTorch) |

## 3. Literature by component

### 3.1 OTA ladder (IR3109)
- Stilson and Smith 1996, Analyzing the Moog VCF (delay-free loop, root locus).
- Huovilainen 2004 DAFx, Non-linear digital implementation of the Moog ladder filter (tanh per stage, oversampling); Valimaki and Huovilainen 2006 CMJ.
- Zavalishin, The Art of VA Filter Design rev 2.1.2 (TPT/ZDF, OTA and ladder chapters, Newton solution).
- D'Angelo and Valimaki 2014 TASLP, Generalized Moog ladder Parts I and II.
- Pirkle, Designing Software Synthesizer Plugins in C++ and app notes (Korg35, diode ladder TPT).
- Paschou, Esqueda, Valimaki, Abel 2017 APSIPA, Modeling and measuring a Moog VCF (measurement + fitting method, directly transferable).
- EDP Wasp OTA VCF model, DAFx 2022 (closest published OTA filter model).
- Nodal DK: Yeh, Abel, Smith 2010 TASLP; Holters and Zolzer 2015 EUSIPCO; DAFx-11 wah with variable parts.
- WDF: Fettweis 1986; Werner 2016 PhD; Werner et al. DAFx-15 R-type adaptors; WDF op-amps.
- OTA nonlinearity: I_out = I_abc tanh(V_diff / 2Vt); KVR threads on slew limiting in closed loop and "cheap non-linear zero-delay filters" (mystran).
- Antiderivative antialiasing: Parker, Zavalishin, Le Bivic DAFx-16; Bilbao, Esqueda, Parker, Valimaki 2017 SPL; Holters 2019 DAFx (stateful ADAA); DAFx-25 recurrent ADAA.
- Chowdhury ADC 2020, comparison of VA techniques.

### 3.2 BBD chorus
- Raffel and Smith DAFx-10, Practical modeling of bucket-brigade device circuits.
- Holters and Parker DAFx-18, A combined model for a BBD and its input and output filters (basis of jpcima and chowdsp code).
- Huovilainen DAFx-05, Enhanced digital models for analog modulation effects.
- Gabrielli, D'Angelo, Squartini DAFx-25, Antialiasing in BBD chips using BLEP.
- Mitcheltree et al. 2023, Modulation extraction for LFO-driven audio effects (recover LFO rate/shape from recordings).
- Electric Druid chorus study (clock, delay, filter measurements).

### 3.3 DCO
- Circuit: Thea Flowers; Electric Druid; synthnerd. Model: integer-period reset (pitch quantisation grows with frequency), slope set by a stepped CV with imperfect tracking, finite reset time.
- Antialiasing: Valimaki and Huovilainen 2007 (PolyBLEP); Nam, Valimaki, Abel, Smith 2010 TASLP; Brandt 2001 minBLEP; Pekonen and Valimaki. Model as BLEP/PolyBLEP saw with the exact integer period rather than an ideal frequency.

### 3.4 Envelopes
- IR3R01 as RC-type stepped ADSR; one-pole per segment models; KVR "Virtual Analog: Envelopes"; Electric Druid VCADSR notes.
- 106 software envelope: integer accumulator at a fixed tick (j106roms, KR-106 "firmware integer" mode).

### 3.5 Grey/black-box calibration
- Wright, Damskagg, Valimaki DAFx-19 RNN black-box; Wright PhD.
- Esqueda et al. DAFx-21, Differentiable white-box virtual analog modeling (learn component values).
- Comunita et al. 2025, differentiable black and grey-box; NablAFx.
- Eichas and Zolzer DAFx-16/17 block-oriented grey-box with sweep + noise excitation.
- Parker et al. DAFx-19 state-space NN (includes MS-20 filter).

### 3.6 Voice-to-voice tolerance
No dedicated paper. Practical: KR-106 per-voice offsets (cutoff +-5 %, pitch +-3 cents, env +-8 %, VCA +-0.5 dB); pendragon Unison folder (measured spread); Roland Condition; Diva voice accuracy; US patent 10725727 (tolerance modelling). On the Juno all voices share the clock, so unison pitch is essentially identical; thickness comes from per-voice VCF/VCA/ramp tolerances.

## 4. Measurement methodology

### 4.1 Isolating blocks
- Calibrate the unit per Service Notes before measuring (model the design), then optionally measure uncalibrated for "condition" data.
- Per-voice capture: on the 106 use test mode; on the 60 use DCB notes with arpeggio off, or probe voice-board test points.
- Chorus: tap the voice sum before the chorus, or feed external audio into the board.

### 4.2 Signals
- Filter: DCO off, VCA gate, env depth 0; inject at the voice-board test point or use the DCO saw as a harmonic comb. Exponential sine sweeps (Farina; synchronized swept sine, Novak 2015) at low level for the linear response and high level for harmonic separation. Grid: cutoff >= 16 positions x resonance >= 8 x key tracking 0/50/100 % x 3 notes. Self-oscillation frequency and amplitude vs cutoff at full resonance to fit the expo converter and feedback gain.
- DCO: saw, pulse (PW 0/25/50/75/100 %), sub, noise at every octave C1 to C6 plus a few in-between notes. Measure exact period (recover N and master clock), ramp amplitude vs pitch, reset time and overshoot, saw curvature, PWM threshold mapping. Record LFO-to-pitch and bend-to-pitch.
- Envelopes: VCA env mode, saw, filter open; A/D/S/R at >= 9 slider points each with a gate from DCB; fit time constants. VCF ENV depth at fixed cutoff for env-to-cutoff scaling.
- Chorus: 1 kHz sine and sweeps; extract LFO rate/shape, delay range (pitch deviation), BBD clock from residue and sidebands in a 192 kHz capture, pre/post filter responses, noise floor, dry/wet ratio, each mode and both channels.
- HPF: sweep at each of 4 positions. Noise: long capture for spectral tilt.

### 4.3 Gear
- 24-bit, 192 kHz capable interface, low-noise balanced line inputs. DC-coupled card or oscilloscope for CVs.
- Oscilloscope >= 50 MHz, 2+ channels, frequency counter for master clock, 8253 reset pulses, BBD clock, MN3101 phases. Voltmeter.
- MIDI interface for the 106; for the 60 a DCB interface (Kenton Pro DCB MK3, Valpower adaptor) gives note on/off.
- Software: REW or Python/scipy for sweep deconvolution; Sonic Visualiser; ACME.jl or ngspice for reference simulation.

### 4.4 Automating a Juno-60
- Notes/gates: DCB (stock) or Tubbutec Juno-66 (replaces the CPU and changes behaviour; prefer stock + DCB for reference).
- Panel parameters are analogue sliders read by the CPU and recalled from memory; neither DCB nor Juno-66 can set them. Options: store slider-grid states as patches and recall them; step sliders by hand with a jig; inject CVs at voice-board test points. On the 106, SysEx 0x32 sweeps every parameter.

## Key unverified items
- Exact 106 SysEx switch-byte bit layout and 0x31 format.
- Juno-60 HPF corners and 106 HPF values.
- Whether the 106 has a distinct I+II chorus mode.
- Sequence15's claims about VCA loading and cap types as the 60/106 tonal cause.
- 106 CPU envelope and LFO update rate.
- MIDI function mode I vs II split.

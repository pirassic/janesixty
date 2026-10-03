# Juno-60 circuit and signal-flow analysis

Status: research notes, compiled 2026-10-03. Tags on every claim:

- **[SN]** quoted from the Juno-60 Service Notes by a secondary source (not read first-hand in this session; the PDF hosts were unreachable from the sandbox)
- **[Thea]** Thea Flowers, *The design of the Roland Juno DCO*
- **[meas]** bench measurement of a real Juno-60 (pendragon-andyh/Juno60 unless noted)
- **[clone]** derived from the gligli/juno-chorus-clone KiCad netlist (Juno-60 chorus board clone)
- **[106]** Juno-106 documentation (sibling, may differ)
- **[plugin]** measured against Roland's own JUNO-60 plugin (model-derived, not hardware)
- **[est]** derivation or estimate

Untagged numbers are arithmetic on tagged values.

## 1. Block diagram

```
MASTER OSC (LC + varicap; BENDER + LFO + TUNE CV)  --clock-->  8253 counters (6 ch) --> per voice:
      DCO ramp core -> waveshaper (saw / PWM pulse / sub) -> MIXER (+ shared NOISE)
      -> VCF (IR3109 + BA662 resonance) -> VCA (BA662)
                                    |
      SUM of 6 voices  <------------+
      -> HPF (one, 4-position, HD14051B switched caps)
      -> VCA "LEVEL" (one, shared, patch level)
      -> CHORUS (2 x MN3009)  -> VOLUME -> output level switch -> jacks (L/mono, R, phones)
```

Per voice (x6): 8253 channel, integrator/reset/waveshaper, mixer, IR3109 VCF, BA662 resonance OTA, BA662 VCA, IR3R01 envelope, DCO-CV sample/hold. Six IR3109s are IC2, 5, 8, 11, 14, 17 [SN].

Shared (x1): master LC clock + varicap, LFO with delay, noise generator, HPF, patch-level VCA, chorus board, bender board (OPH162A), output amp, two CPUs.

Two easily missed facts [SN p.3]: the HPF sits **after** the voice sum, and the patch LEVEL VCA sits **before** the chorus, so hot patches drive the BBDs harder (no compander on the board).

Tuning reference in the Service Notes is **A4 = 442 Hz** (master osc adjustment, bender adjustment, key figure, DCB table) [SN]. The plugin should default to 440 and note the +7.85 cent offset.

## 2. DCO

### 2.1 Master clock and dividers

- No crystal. An analogue **LC oscillator with varicap D18** receives the summed BENDER, LFO and TUNE control voltages, common to all voices [SN, Thea]. TR58 to TR62 form the oscillator [SN].
- Nominal ~**1.902 MHz** (Electric Druid, snippet); modulation sweeps roughly 1 to 3.5 MHz [Thea].
- Modulation ranges [SN]: BENDER +-700 cents, LFO +-300 cents, TUNE +-50 cents; summed maximum +-1050 cents. Bender calibration: lever hard left, E5 reads 442 Hz; hard right, D4 reads 442 Hz (exactly +-7 semitones).
- Consequence: bend and vibrato are applied **to the clock**, so they are perfectly common to all six voices, continuous (no zipper), and exponential (cents). PWM-by-LFO is likewise common.
- Dividers: Intel **8253** 16-bit programmable interval timers, one channel per voice, divide ratio N set by the CPU [Thea]. The counter output edge resets the ramp. Two 8253s give six channels. Counter mode and rounding rule are not settled.
- Pitch resolution [est, 1.902 MHz, integer N]: step = 1200 log2((N+1)/N) cents.

| Note | N | step (cents) |
|---|---|---|
| C2 65 Hz | ~29078 | 0.06 |
| A4 442 Hz | ~4303 | 0.40 |
| C7 2093 Hz | ~909 | 1.9 |
| C8 4186 Hz (4' top key) | ~454 | 3.8 |

Worst-case rounding error is half a step. Keyboard: 61 keys C2 to C7, ranges 16'/8'/4'.

### 2.2 Ramp core (per voice)

- Inverting op-amp integrator (half a TL082-class), integrating capacitor **C7 = 1 nF** [SN p.9, Thea]. The ramp **falls** (0 V to about -12 V) because the charge current is positive [Thea].
- Reset: PNP **2SA1015 (TR5)** across the capacitor via a small series resistor. Thea reads **2.2 kOhm** (RC 2.2 us); another reading of the Juno-6 drawing gives "2.2 Ohm". Unsettled. Thea notes the op-amp output slew also drives the flyback, so the real retrace is ~1 us class either way.
- Reset pulse: RC differentiator **C6 = 270 pF, R34 = 10 kOhm** (tau 2.7 us) on the counter output edge [Thea, SN]. PNP triggers on the falling edge; on for roughly 5 us per cycle, ~1 to 2 % of the period at the top of the range [Thea].
- Expected imperfections [Thea]: finite reset time gives a short flat/retrace glitch each cycle; incomplete discharge adds DC offset and amplitude error; the saw is analogue and alias-free, so the model must generate a band-limited ramp with a finite-slope reset.

### 2.3 Amplitude compensation (DCO CV)

- The integrator input is a CPU-generated **DCO CV via DAC**, higher for higher notes, so ramp slope scales with frequency and saw amplitude stays roughly constant [Thea]. The 106 Service Notes say "approximately 12 Vp-p over the frequency range", with ~1 V variation [106].
- Because DAC resolution is limited, the integrator resistor is switched with RANGE (16'/8'/4') via a **4052** analogue switch: 100 k / 200 k / 399 k on the 106 [Thea, Electric Druid]. The Juno-60's resistor set is not confirmed.
- The CV is a per-key staircase, so saw amplitude vs pitch is a **staircase**, not a smooth curve. Per-voice S/H vs per-voice DAC is unsettled (the 106 uses a shared DAC + S/H).
- The DCO CV also scales the pulse comparator drive so PWM duty is nominally pitch independent [106].

### 2.4 Waveshaping

- **Saw**: taken directly from the integrator (falling ramp, ~12 Vp-p). The "Juno saw shape" is the ramp plus finite flyback plus any incomplete-discharge offset.
- **Pulse**: ramp compared against the PWM CV by a TL08x inverting comparator [Thea]. 106 text: duty 50 % at +6 V PWM CV, **95 %** at +0.6 V. So the width runs square to 95 % over the slider; 50 % is the minimum. Sources: MAN (slider = fixed width), LFO (shared), ENV (per voice). Slider to duty is a raised cosine 0.5 to 0.95 and there is **no level compensation**: a narrow pulse loses level as 2 sqrt(d(1-d)), -8.6 dB at narrowest [plugin].
- **Sub**: square one octave down from a D flip-flop divide-by-two (CD4013-class) on the counter output [Thea]. Clean 50 % square, phase-locked to the saw reset.
- Polarity matters: the falling saw's fundamental and the pulse's fundamental **add**; a rising ramp would cancel ~14 dB of odd harmonics when both are on [plugin].
- The 106 does the same functions in the MC5534A waveshaper IC; the 60 uses discrete op-amps and the 4013.

### 2.5 Firmware-related

- Oscillators free-run; not reset on key-on.
- All six voices derive pitch from one clock, so unison is a true unison with no beating except counter rounding (under 2 cents at the top) and per-voice amplitude/offset tolerances.

## 3. Mixer

- Inputs per voice: saw (switch), pulse (switch), sub (slider), noise (slider, one shared source). Resistive sum into the VCF input; the Juno-6/60 p.9 drawing shows **10 k into the 68 k / 560 Ohm** IR3109 input attenuator [SN p.9]. Per-leg mixer resistor values not readable from available sources.
- Relative levels [plugin]: pulse ~-1.3 dB vs saw, sub ~-1.3 dB, noise ~-1.8 dB; sub and noise sliders follow a two-segment taper (lower half linear at 0.406 of setting). MKS-7 notes: saw 4.8 Vp-p +-0.5 V at VCA output, pulse/saw 0.79 to 0.83 at the mix [106]. Bracket only.
- Noise: "low-passed at ~5 kHz" comes from AR80017A (106 clone) notes. Unconfirmed for the 60. Tubbutec found Juno-6/60 noise quality limited by +5 V rail ripple coupling, so the real noise has a digital/supply component.
- No diode clipper documented in the mixer. First nonlinearity is the IR3109 input OTA; the 68 k / 560 attenuator (divide by 122) means a 12 Vp-p saw reaches ~100 mVp-p at the pair, about +-2 Vt, mild tanh compression [est].

## 4. VCF (IR3109)

### 4.1 Topology and values

- **IR3109**: quad OTA + four P-MOS buffers + one on-chip exponential current source driving all four OTAs. Four cascaded one-pole OTA-C sections, 24 dB/oct LP; each pole 45 deg at cutoff, 180 deg total, so resonance feedback is **inverting**.
- Standard Roland values, confirmed for the Juno-6/60: **68 kOhm** input resistor per stage, **560 Ohm** to ground at each OTA (+) input, **240 pF** integrating cap per stage (ceramic disc). f_c = g_m / (2 pi 240 pF).
- Resonance: external **BA662** OTA feeds the inverted stage-4 output back to stage 1. BA662 has no public datasheet; parts were binned by offset ("white dot"). Juno-6/60 p.9 network: stage-1 series input 10 k into the 68 k node, resonance OTA leg 47 k, plus 100 k, 1.5 k, 560 Ohm, and a 4.7 k VCA input. Q-compensation coefficient read from the drawing: (10/68)(101.5/48.5) = **0.308**.
- **Q compensation, input side**: part of the input signal is routed through the resonance BA662 so raising resonance also raises drive into stage 1, offsetting passband drop (Roland/Jupiter-8 approach; the SH-101 compensates at the output). Passband still loses ~**7.2 dB** between zero and full resonance [plugin].
- **Self-oscillation**: yes. Onset at roughly 75 to 90 % of slider travel. Amplitude limited by OTA tanh, soft, mild harmonics (no diode clamp as on the SH-101). Trackable as a sine over ~8 octaves. Factory bank 7 uses it as the only sound source.
- Calibration anchor [SN]: **248 Hz self-oscillation with FREQ at 3/10, max resonance, max key follow**. FREQ pot 50 k linear, no loading network. Key-follow pivot **C4** (no cutoff change at C4) on the 60 and 106. The 106 ROM gives 1143 DAC counts/octave; the 60's counts/octave not confirmed.
- Resonance self-oscillation level trim: 106 procedure sets 4.8 Vp-p per voice; the 60's figure not seen.

### 4.2 Cutoff CV summing

Cutoff CV = FREQ + ENV x depth x polarity + LFO x depth + KEY FOLLOW (0 to 100 %, 1 V/oct equivalent at 100 %), summed into the IR3109 exponential input; tempco resistor in the CV path. On the Juno-60 every slider is digitised by the CPU and regenerated by DAC + multiplexer, so CVs are stepped. Hardware depths not documented; against the plugin: ENV full depth ~**10.8 to 11 octaves**, LFO ~**+-3.6 octaves**, both through a strongly curved slider taper (~1 % of full scale at slider 0.1, 5 % at 0.2). Cutoff range: 106 spec 5 Hz to 50 kHz; Juno-6 spec 4 Hz to 40 kHz. Plugin slider spans ~12 Hz to 18 kHz.

### 4.3 Nonlinearity model

Each stage: I_out = I_ctrl tanh((V_in - V_out) / (2 Vt)), Vt ~26 mV. Use TPT one-poles with tanh in each stage and the feedback path, Newton or implicit solve; k = 4 at self-oscillation. Per-stage buffer drive differs in the AS3109 clone (stages 1/3 ~0.6 mA, 2 ~1.0 mA, 4 ~1.3 mA). 240 pF ceramic mismatch spreads the poles per voice.

### 4.4 HPF (shared, after the sum)

- 4-position slider; **HD14051B** selects one of three capacitors **0.022 / 0.01 / 0.0047 uF** or bypass [SN]. Corners 1/(2 pi R C) = **154 / 339 / 720 Hz**, one pole, 6 dB/oct; implied R = **47 kOhm** [est].
- Position 0: schematic reading says **flat** on the Juno-60. The 106 adds a +6 dB low shelf below ~65 Hz at position 0. One forum claim says the 60 also boosts; the schematic reading is better sourced. Bench check required.

## 5. VCA

- Per voice: **BA662** OTA, gain linear in control current, driven by the IR3R01 envelope (ENV) or the raw gate (GATE). Output load R42 = 47 k, input via 4.7 k [SN p.9].
- Per-voice **VCA OFFSET trimmer VR5** nulls CV feedthrough (thump). Residual offset gives envelope-shaped DC thump and gate-mode clicks.
- GATE mode is not a hard step: ~**3 ms rise, 6 ms fall** [meas].
- 106/MKS-7 window: voice VCA output 6.0 Vp-p from a 4.8 Vp-p filter test signal. Bracket only.
- Shared patch LEVEL VCA: before the chorus; slider law measured as squared [plugin]. Hera models it as 0.1 x 1.2589^(10x) (20 dB range).

## 6. Envelope (IR3R01, per voice)

- Roland custom **IR3R01** (also Jupiter-8). Time CVs set an internal oscillator frequency exponentially which steps the charge/discharge (Jupiter-4 lineage). Timing cap **47 nF** on the Juno-6/60; reference +7.5 V; output 0 to ~7.5 V (another source says 10 V, unsettled); D/R CV input resistors 22 k; A/D/R CVs 0 to 5 V, Sustain CV 0 to 10 V; per-voice trimmer on the common pin. Juno-60 procedure 10: **ENV TIME VR6 set for a 3 s attack** [SN]. Gate and Trigger tied: every note-on restarts attack **from the current level**.
- Spec: Attack 1 ms to 3 s; Decay 2 ms to 12 s; Release 2 ms to 12 s.
- Measured on a real Juno-60 [meas], slider 0 / 2.5 / 5 / 7.5 / 10:

| Segment | 0 | 2.5 | 5 | 7.5 | 10 |
|---|---|---|---|---|---|
| Attack (s) | 0.001 | 0.03 | 0.24 | 0.65 | 3.25 |
| Decay, S=0 (s) | 0.002 | 0.096 | 0.984 | 4.449 | 19.78 |
| Release (s) | 0.002 | 0.096 | 0.984 | 4.449 | 19.78 |

  Decay with S=5 at slider 10: 17.11 s. **Decay duration is essentially independent of sustain level** (not RC-toward-target; consistent with the stepped design). Unit drifted ~8 % over spec.
- Attack shape at slider 10: level(t) = (1 - e^(-t/T)) / 0.632, i.e. one time constant of an exponential, truncated at 63 % and renormalised (an RC charging toward ~1.58x target).
- Decay/release shape: exponential, about -40 dB in the nominal time; fit level = S + (1 - S) e^(-4.6 t/T).
- Slider to time is logarithmic. Sustain slider to level: 1 - (1 - s)^1.6 [plugin, needs hardware confirmation].
- Front panel A/D/R sliders on the Juno-6: 50 k linear with 4.7 k shunt giving a reverse-log taper that cancels the chip's exponential law. On the 60 the sliders are CPU-scanned, so the equivalent law lives in firmware/DAC. Unverified.
- Retrigger during attack restarts from current level. Hera's notes claim release during attack passes through a decay-like stage. Unverified.

## 7. LFO (shared)

- Triangle core, op-amp integrator + Schmitt comparator (TL062 / NJM072B class). Spec RATE 0.3 to 20 Hz; factory adjustment sets slider top to a 45 ms period (22 Hz). DELAY TIME VR4 set so modulation disappears on key press and reappears 2 s later; owner's manual says 0 to 2 s; service spec page says 1.5 s. LFO OFFSET trimmer VR3.
- Measured [meas], slider 0 / 2.5 / 5 / 7.5 / 10: rate **0.3 / 0.85 / 3.39 / 11.49 / 22.2 Hz**; delay hold **0 / 0.064 / 0.85 / 1.2 / 2.69 s** then fade-in **0.001 / 0.053 / 0.188 / 0.348 / 1.15 s**. "Delay" is a dead time then a ramp of depth.
- Destinations: DCO pitch (via master clock varicap, +-300 c max, shared), VCF cutoff (per voice summer), PWM (shared).
- Trigger: AUTO/MAN switch and LFO TRIG button. AUTO: delay envelope restarts on the first key of a phrase (no other keys held). MAN: modulation only while the button is held. Whether the triangle phase resets is **not settled**.
- DCO LFO depth law: depth proportional to slider squared, reaching ~3.9 semitones [plugin] vs +-300 c [SN].

## 8. Chorus (shared, stereo out)

### 8.1 Hardware

- Two **MN3009** 256-stage BBDs (one per channel), each with an **MN3101** clock driver, fed from one triangle LFO; right channel modulation **inverted** in modes I and II; in **I+II** both lines get the same modulation and output is effectively mono. Rails +-15 V.
- Service Notes label rates 0.5 / 0.83 / 1 Hz; the 1 Hz is a typo, measured 9.75 Hz. Juno-6 notes: 0.4 / 0.67 / 8.06 Hz.

### 8.2 Measured [meas]

| Mode | LFO | delay sweep | output |
|---|---|---|---|
| I | 0.513 Hz triangle | 1.66 to 5.35 ms | stereo, R inverted |
| II | 0.863 Hz | same | stereo, R inverted |
| I+II | 9.75 Hz | 3.30 to 3.70 ms | both in phase (mono vibrato) |

BBD clock = 256 / (2 x delay): **77 kHz to 24 kHz** for I/II, 39 to 35 kHz for I+II. BBD Nyquist dips to ~12 kHz at the long end. Holters and Parker measured the Juno-60 BBD path gain at ~+2.3 dB. Dry/wet: Hera uses dry 0.83, wet 1.0; summer resistors give wet/dry = 47/39 = 1.205; a Juno-6 fit gives dry 0.863, wet 1.257. Off = dry only (JFET mute on wet).

### 8.3 Topology from the clone netlist [clone]

- **Input**: TL082 buffer (33 k to ground), 100 k level pot, non-inverting gain x(1 + 33k/10k) = 4.3. Feeds both dry legs and the BBD pre-filter.
- **Pre-BBD anti-alias filter** (shared): two unity-gain Sallen-Key LP sections with 2SA1015 emitter-follower buffers, R = 22 k / 22 k each: section A 820 pF / 680 pF (f0 ~9.7 kHz, Q 0.55); section B 1.8 nF / 270 pF (f0 ~10.4 kHz, Q 1.29); then 10 k series + 2.2 nF to ground at each MN3009 input (7.2 kHz real pole), plus a bias trimmer. jpcima's schematic-derived pole set matches: real 7.41 kHz, pairs 9.69 kHz Q 0.55 and 10.34 kHz Q 1.24; response -1 dB 3.6 kHz, **-3 dB ~6.5 kHz**, -12 dB 11.7 kHz, -24 dB 16 kHz, -40 dB 23.5 kHz. KR-106's ngspice run claims -3 dB at 9.66 kHz. Disagreement, needs measurement.
- **Clock VCO** (per channel): LFO voltage to V-to-I stage to a current-controlled relaxation oscillator charging **150 pF**, injected into the MN3101 OX pins. On the 106 the equivalent gives clock period affine in LFO voltage, so **delay linear in LFO**, clock hyperbolic in LFO. Same topology here; triangle-in-delay is the right default.
- **LFO**: TL082 Schmitt (47 k / 33 k, triangle peak ~+-9.5 V) + integrator (1 M, 100 nF); buffered triangle plus inverting unity stage for the anti-phase channel. Original mode-switch resistors not recoverable from the clone.
- **Post-BBD**: both BBD output phases summed through 3.3 k + 3.3 k into 47 k parallel 2.2 nF (clock ripple cancel, ~28 to 45 kHz pole), then the same two Sallen-Key sections, 1 uF coupling, **2SK30 JFET mute** (gate driven by a slow RC: 2.2 uF with 150 k / 560 k / 330 k, delayed fade on chorus switching), then 39 k into the summer. jpcima's output set: pairs 8.87 kHz Q 0.54, 10.38 kHz Q 1.24, real 28 kHz; -3 dB ~8.8 kHz.
- **Output summers**: two TL082 inverting summers, feedback 10 k, dry via 47 k, wet via 39 k, 1 k series to jacks (Juno-6 uses 100 k feedback). L = dry + BBD1, R = dry + BBD2.
- Noise: no compander, so BBD noise and clock residue are a constant hiss and distortion is level dependent; BBD transfer loss grows as the clock slows, giving slight AM at the LFO rate.

## 9. CPU, voice assignment, DCB

- Two NEC 8049-family MCUs: main **uPD8049C-238** (early; bug with lowest C in Transpose LOW with long release) or **-380** (fixed); **D80C49C-028** on Panel Board B scanning TRANSPOSE and the programmer switches. Main CPU scans the keyboard matrix, programs the 8253s, drives the DACs / 4051 demultiplexers for DCO CV and patch CVs, arpeggio clock, DCB.
- Key scan rate / note latency: not found (106 converter pass is 4.2 ms). Measure.
- Assignment [SN]: cyclic; the 7th key steals the 1st (oldest) voice. Test mode (power on with KEY TRANSPOSE held) exposes three modes via the ARPEGGIO MODE switch: UP = mono (all 6 voices to latest key); UP/DOWN = rotary (next free voice in numeric order, cycling); DOWN = non-rotary (lowest-numbered free voice). HOLD and arpeggio work in test mode.
- Arpeggio: UP / UP&DOWN / DOWN, 1 to 3 octaves, 1.5 to 50 Hz, rear clock input; up/down does not repeat the turnaround note; each step is a gate (~55 % gate fraction [plugin]). Arpeggio switches and volume are not part of the patch.
- No portamento in hardware.
- DCB: 14-pin; 3 wires each way + ground, 5 V TTL, Rx-Busy open collector; serial 31.25 kbaud, LSB first, 8 data bits, 2 stop bits, odd parity; Key Code identifier FEh followed by 6 data bytes (one per voice: bit 7 gate, bits 0 to 6 key number). The Juno-60 recognises only Key Code. Bidirectional, no master/slave.
- Trimmers: PSU VR1 (-15 V), VR2 (+5 V); master osc L1; bender; LFO rate, LFO OFFSET VR3, DELAY TIME VR4; ENV TIME VR6; per-voice VCA OFFSET VR5; per-voice VCF 248 Hz anchor (and probably resonance level). Model effects: per-voice spread in cutoff offset, resonance onset, envelope time, VCA thump, saw amplitude.

## 10. Output stage

VOLUME pot, output amp, level switch L -30 dBm / M -15 dBm / H 0 dBm, OUTPUT L/mono and R, PHONES. 1 k series on chorus board outputs [clone]. Internal brackets: DCO saw ~12 Vp-p; VCF test signal 4.8 Vp-p; VCA out 6 Vp-p; chorus input gain 4.3 after a level pot; BBD optimum bias ~-8.3 V.

## 11. Confident component list

| Block | Item | Value / part | Tag |
|---|---|---|---|
| CPU | MCUs | uPD8049C-238 / -380; D80C49C-028 | SN |
| DCO | dividers | Intel 8253 | Thea |
| DCO | master clock | LC + varicap D18, TR58-62, ~1.902 MHz, L1 trim | SN |
| DCO | integrator cap | C7 1 nF | SN p.9 |
| DCO | reset | 2SA1015 TR5; series 2.2 k (Thea) or 2.2 Ohm (other reading) | conflict |
| DCO | reset differentiator | C6 270 pF, R34 10 k | Thea, SN |
| DCO | op-amps | TL082 class | Thea |
| DCO | range resistor switch | 4052; 100 k / 200 k / 399 k (106 values) | Thea |
| DCO | sub divider | 4013-class D flip-flop | Thea |
| VCF | core | IR3109 x6; 68 k in, 560 Ohm, 240 pF x4 | SN |
| VCF | resonance OTA | BA662; 10 k, 47 k, 100 k, 1.5 k, 560 Ohm | SN p.9 |
| VCF | FREQ pot | 50 k linear | SN |
| VCA | per voice | BA662; 4.7 k in; R42 47 k load; VR5 | SN p.9 |
| ENV | per voice | IR3R01; 47 nF; VR6 | SN |
| HPF | | HD14051B; 0.022 / 0.01 / 0.0047 uF; R 47 k derived | SN |
| LFO | ICs | TL062 / NJM072B; VR3, VR4 | SN |
| Chorus | BBD / clock | 2 x MN3009, 2 x MN3101 | SN |
| Chorus | filters | Sallen-Key 22k/22k, 820p/680p and 1.8n/270p, x2 pre and x2 post; 10k + 2.2n at BBD in; 3.3k+3.3k into 47k parallel 2.2n at BBD out | clone |
| Chorus | summer | 10 k fb, 47 k dry, 39 k wet | clone |
| Chorus | mute | 2SK30 JFETs; 2.2 uF / 150 k / 560 k | clone |
| Chorus | clock VCO cap | 150 pF | clone, 106 p.15 |
| Bender board | | OPH162A | SN |

## 12. Not settled by schematics: needs bench measurement

1. DCO saw amplitude vs pitch and range (DAC staircase, 60's integrator R set, per-key error, DC offset from incomplete discharge, flyback shape and duration).
2. Mixer resistor values and absolute levels into the VCF (drive level sets tanh colouring).
3. IR3109: g_m vs control law and tempco, OTA saturation level, cutoff slider to Hz curve for the 60 (only the 248 Hz anchor is documented), ENV and LFO depth in octaves, Q-compensation coefficient, resonance onset, self-oscillation amplitude and trim, passband loss vs resonance.
4. Envelope: IR3R01 output swing, attack overshoot ratio, A/D/S/R slider quantisation on the 60, sustain law, retrigger behaviour, decay-vs-sustain independence (one unit measured).
5. VCA: BA662 control law, offset bleed after trim, GATE edge times, distortion at full drive.
6. LFO: triangle linearity, phase reset on trigger, delay hold/fade law, minimum rate.
7. Chorus: BBD clock range per mode, clock VCO law, absolute wet gain, pre-filter corner (6.5 vs 9.7 kHz), noise floor and clock residue spectrum, saturation, mute fade time, I+II exact depth.
8. HPF position 0: flat vs bass boost.
9. Noise generator spectrum and supply coupling.
10. Firmware: key-scan period, note-on latency, 8253 mode and rounding, DCO-CV DAC resolution, patch parameter resolution, arpeggio gate fraction, DCB timing.
11. Output stage: absolute dBm vs VOLUME, output impedance, roll-off.

## Sources reached directly

- Thea Flowers, source Markdown of *The design of the Roland Juno DCO*: https://github.com/theacodes/blog.thea.codes (published at https://blog.thea.codes/the-design-of-the-juno-dco/)
- pendragon-andyh/Juno60 (real-unit measurements): https://github.com/pendragon-andyh/Juno60
- Michi71/PicoVintageSynthCollection, PicoFaceJ6 (Service Notes citations, Roland plugin measurements): https://github.com/Michi71/PicoVintageSynthCollection
- jpcima/Hera (bbd_filter pole sets, chorus, tables, envelope): https://github.com/jpcima/Hera
- gligli/juno-chorus-clone (KiCad netlist): https://github.com/gligli/juno-chorus-clone
- kayrockscreenprinting/ultramaster_kr106 (docs/juno6-vcf-reference.md, juno6-adsr-reference.md, vcf_modulation_budget.md): https://github.com/kayrockscreenprinting/ultramaster_kr106
- protocodus/virtual-instrument-youknow (Service Notes p.9 readings): https://github.com/protocodus/virtual-instrument-youknow
- mattWoolly/mwAudio101 (IR3109 / BA662 notes): https://github.com/mattWoolly/mwAudio101

Snippet-only (not fetched): Juno-60 Service Notes (Apr 1983); Electric Druid "Roland Juno DCOs" and IR3109 filter pages; AMSynths IR3R01 / IR3109 / BA662 pages; CHD DCB application note; hyperreal Juno-60 test-mode text; Roland Juno-60 technical specifications; Juno-106 Service Notes; Tubbutec noise article; sequence15 60-vs-106 article; Holters and Parker DAFx-18.

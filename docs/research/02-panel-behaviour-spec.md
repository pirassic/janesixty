# Juno-60 panel and behaviour specification

> **Corrections (2026-10-04):** the owner's manual has since been read first-hand; see `05-manual-verified-facts.md`, which supersedes this file where they conflict. Known errors below: there is **no RANGE 16'/8'/4' switch** (section 2.4 and section 3); **OCTAVE TRANSPOSE is stored in the patch** (section 3); the edit indicator is **both decimal points**; the octave switch legend is DOWN / NORMAL / UP; the factory patch chart has an octave-transpose column (see 05, section 4) and chorus I+II is a stored value.

Status: research notes, compiled 2026-10-03. The owner's manual PDF itself was not readable from this sandbox (host blocked). Content comes from Roland's published spec sheet, search-engine snippets of manual text, the Service Notes as quoted by secondary projects, bench measurements (pendragon-andyh/Juno60) and the factory patch sheet transcription in dzannotti/junox. Items marked **[UNVERIFIED]** or **[AMBIGUOUS]** need checking against the PDF; see the checklist at the end.

## 1. Architecture summary

```
MASTER OSC (TUNE, BENDER, LFO)
 -> DCO x6 -> waveshaper (saw / pulse / sub / noise mix)
 -> VCF x6 (IR3109, 24 dB/oct LP, resonance to self-oscillation)
 -> VCA x6 (BA662, ENV or GATE)
 -> SUM (mono)
 -> HPF (one, 4 positions, after the sum)
 -> VCA LEVEL (patch level, before the chorus)
 -> CHORUS (2 x MN3009, one triangle LFO, R channel inverted) -> stereo
 -> VOLUME -> OUTPUT L(mono)/R, PHONES
```

6 voices, one DCO and one ADSR per voice (the single ADSR serves VCF, VCA and PWM-ENV), one shared LFO, HPF and chorus. Service Notes tuning reference A4 = 442 Hz. Master oscillator ranges: BENDER +-700 cents, LFO +-300 cents, TUNE +-50 cents, summed maximum +-1050 cents.

## 2. Panel, left to right

### 2.1 POWER / VOLUME
- POWER switch with LED. VOLUME rotary, master output after chorus.
- TUNE is on the **rear** panel, +-50 cents.

### 2.2 BENDER panel (below the main panel, left of the keys)
Spec sheet items: Bend Sens (DCO), Bend Sens (VCF), BENDER lever, LFO Trigger button, OCTAVE TRANSPOSE switch (DOWN / NORMAL / UP).
- **BENDER lever**: spring return, left/right only. No forward push (that is a 106 feature).
- **DCO** slider: lever pitch depth, max +-700 cents (+-7 semitones).
- **VCF** slider: lever cutoff depth, bipolar. Numeric depth not published [UNVERIFIED].
- **LFO TRIG** button (momentary): with the LFO switch at MAN, LFO modulation is applied while held. Whether PWM-by-LFO is also gated and whether the delay restarts per press is [AMBIGUOUS]; junox and Roland's PG-JU60 gate all LFO output and restart the delay.
- **TRANSPOSE** 3-position: DOWN / NORMAL / UP = -1 / 0 / +1 octave. Performance control, not stored. Printed legend may read "L M H" [AMBIGUOUS].

### 2.3 LFO
- **RATE** 0 to 10: 0.3 to 20 Hz (spec), calibrated to 22 Hz at the top. Triangle. junox fit: f = 0.25 x 1.2 x 1.53^(10v) x (1 + 0.39 sin(pi v)).
- **DELAY TIME** 0 to 10: 0 to 2 s (manual). Measured as a silent hold then a fade-in:

| Slider | Hold (s) | Fade-in (s) |
|---|---|---|
| 0 | 0 | 0 |
| 2.5 | 0.064 | 0.053 |
| 5 | 0.85 | 0.188 |
| 7.5 | 1.2 | 0.348 |
| 10 | 2.786 | 1.0 |

- **TRIGGER MODE AUTO / MAN**: AUTO restarts the delay when a key is pressed with no other keys sounding. MAN only via the LFO TRIG button. Phase reset behaviour [UNVERIFIED].

### 2.4 DCO
- **RANGE** 16' / 8' / 4'. Stored in patch.
- **LFO** 0 to 10: vibrato depth, full = +-300 cents (plugin measures ~+-3.9 semitones; roughly square-law taper).
- **PWM** 0 to 10: in MAN sets pulse width (0 = 50 % square, 10 = narrow, ~95 %); in LFO or ENV sets modulation depth.
- **PWM mode** LFO / MAN / ENV. ENV position is Juno-60 only (not on the Juno-6 or 106).
- **Pulse** button + LED, **Sawtooth** button + LED; both may be on.
- **SUB** button + LED, **SUB level** 0 to 10: square one octave below.
- **NOISE** 0 to 10: white noise (low-passed around 5 kHz per 106 clone notes).
- Mix trims against Roland's plugin: pulse and sub ~-1.3 dB, noise ~-1.8 dB relative to saw.
- Digitally clocked: no pitch drift.

### 2.5 HPF
One slider, **four detents 0 1 2 3**, applied once to the summed mono signal, 6 dB/oct.

| Position | Juno-60 |
|---|---|
| 0 | flat (no bass boost; the +6 dB low shelf is a 106 feature) |
| 1 | -6 dB/oct below 154 Hz |
| 2 | -6 dB/oct below 339 Hz |
| 3 | -6 dB/oct below 720 Hz |

### 2.6 VCF
- **FREQ** 0 to 10: cutoff, ~20 Hz to 18 kHz in emulations. Factory patches often sit FREQ near 0 and open with ENV, so ENV at 10 must sweep ~10 octaves.
- **RES** 0 to 10: self-oscillation from ~70 to 80 % of travel. Bank 7 patches use self-oscillation as the only source. Passband loss at max resonance ~7 dB (less than a Moog ladder).
- **Polarity switch** (two envelope pictograms, upright = positive, inverted = negative). Factory charts record N / I.
- **ENV** 0 to 10: ADSR to cutoff depth.
- **LFO** 0 to 10: LFO to cutoff, ~+-3.6 octaves at full.
- **KYBD** 0 to 10: key follow 0 to 100 %, pivot around C4.
- Rear VCF CONTROL jack (FV-200 pedal) adds to cutoff.

### 2.7 VCA
- **ENV / GATE**: GATE = on/off while a key is held (~3 ms rise, ~6 ms fall); the ADSR still drives VCF and PWM.
- **LEVEL**: patch level VCA after the sum, before the chorus. Factory charts record values from -3 to +5, so the printed scale is bipolar with 0 at centre, probably -5 to +5 [AMBIGUOUS]. Hot settings drive the BBDs harder.

### 2.8 ENV
Sliders A, D, S, R, 0 to 10. Spec: Attack 1 ms to 3 s; Decay 2 ms to 12 s; Sustain 0 to 100 %; Release 2 ms to 12 s.

Measured on a real unit:

| Slider | Attack (s) | Decay (s), S=0 | Release (s) |
|---|---|---|---|
| 0 | 0.001 | 0.002 | 0.002 |
| 2.5 | 0.03 | 0.096 | 0.096 |
| 5 | 0.24 | 0.984 | 0.984 |
| 7.5 | 0.65 | 4.449 | 4.449 |
| 10 | 3.25 | 19.783 | 19.783 |

Fits: attack 0.001 + (e^(0.5s) - 1) / (e^5 - 1) x 3.25; decay/release 0.002 + (e^(0.4s) - 1) / (e^4 - 1) x (s/10) x 17.46, s = slider 0 to 10. Factory adjustment sets attack at full to 3 s, so 3.25 is drift. Decay time does not depend on sustain level. Curves: attack (1 - e^(-x)) / 0.632; decay/release S + (1 - S) e^(-3.5x) - e^(-3.5). Sustain slider to level: 1 - (1 - s)^1.6 (plugin-derived).

### 2.9 CHORUS
Buttons OFF / I / II. Pressing I and II together gives the undocumented I+II mode.

| Mode | LFO rate | Delay min | Delay max | Output |
|---|---|---|---|---|
| I | 0.513 Hz | 1.66 ms | 5.35 ms | stereo |
| II | 0.863 Hz | 1.66 ms | 5.35 ms | stereo |
| I+II | 9.75 Hz | 3.3 ms | 3.7 ms | near mono, fast vibrato |

Dry present in both channels; OFF = identical mono on L and R.

### 2.10 ARPEGGIO
- ON/OFF button + LED. MODE: UP / U&D / DOWN. RANGE: 1 / 2 / 3 octaves. RATE 0 to 10: 1.5 to 50 Hz, overridden by the rear ARPEGGIO CLOCK input (1 step per pulse, +2.5 V or more).
- Pattern is rebuilt from the currently held set on each new key. Each step retriggers the envelope (gated). Not stored in patches.

### 2.11 HOLD
- Button + LED; rear PEDAL HOLD jack (DP-2).
- Manual: "the sound remains even after you release the key. The level of the sound is controlled by S." Hold keeps the gate on. "The HOLD function applies up to 6 keys at a time; if more than 6 keys have been played, the last six keys will remain."
- With arpeggio: "If you press HOLD while an Arpeggio is being played, it will continue after the keys are released." Turn ARPEGGIO on before HOLD.

### 2.12 KEY TRANSPOSE
- Button + indicator. Hold the button and press any key: the Juno plays in that key; indicator lit when not C. Hold and press any C (except the highest) to return. Direction of shift [AMBIGUOUS], implied chromatic offset within the octave.
- Undocumented: power-on with KEY TRANSPOSE held and ARPEGGIO MODE up enters a mono mode.

### 2.13 MEMORY
Spec sheet: PATCH NUMBER buttons 1 to 8, BANK buttons 1 to 5, MANUAL, WRITE, SAVE / VERIFY / LOAD buttons with indicators, program number display.
- **56 patches = 7 banks x 8**, numbered 11 to 78. Five physical BANK buttons; banks 6 and 7 by pressing bank 5 together with 1 or 2 [UNVERIFIED wording]. Hidden banks 8 / 9 (bank 5 + 3 / 5 + 4) hold test programs, reachable only with a plug in PATCH SHIFT.
- **Display**: 2-digit 7-segment. Shows bank + patch. A decimal point appears after the number when any control is moved after recall (edited indicator). "--" after a successful tape operation and at CPU reset; "Er" = tape error; "__" while WRITE is held before a destination is chosen.
- **MANUAL**: sound taken from the physical control positions. Display content in manual mode [UNVERIFIED].
- **Recall**: loads all patch parameters (section 3), not arpeggio / hold / transpose / bender / volume. Moving a slider after recall makes that parameter jump to the physical position (no catch-up) and lights the edit dot.
- **WRITE**: MEMORY PROTECT off; hold WRITE ("__"); press BANK then PATCH NUMBER; protect back on. Refused with protect on.
- **PATCH SHIFT** jack (DP-2): steps through the 8 patches of the current bank.
- Battery-backed RAM.

### 2.14 TAPE interface
- **SAVE**: SAVE jack to recorder, record, press SAVE (LED on). All 56 patches as FSK audio, ~45 to 60 s, then LED off and "--".
- **VERIFY**: recorder out to LOAD jack, play, press VERIFY. "--" on success, "Er" on mismatch, memory untouched.
- **LOAD**: MEMORY PROTECT off, as verify but LOAD. All 56 replaced. "Er" on level problems.

## 3. Patch memory contents

Stored: LFO Rate, LFO Delay, LFO trigger mode (A/M), DCO LFO depth, PWM amount, PWM mode (L/M/E), Pulse, Saw, Sub on, Sub level, Noise, HPF 0 to 3, VCF Freq, Res, polarity (N/I), Env, LFO, Kybd, VCA mode (E/G), VCA Level, A, D, S, R, Chorus 0/1/2 (+ I+II), Range 16'/8'/4'.

Not stored: arpeggio on/mode/range/rate, hold, octave transpose, key transpose, bender depths, volume, tune.

## 4. Keyboard and voice assignment

- 61 keys C to C, 6 voices.
- Cyclic (rotary) assignment: next free voice in number order; when all six sound, the 7th key steals the oldest. (The 106 refuses the 7th note instead.) Preference for releasing voices over held ones [UNVERIFIED].
- HOLD: last six keys retained.
- Keys beyond the 61 range via DCB are not clamped.

## 5. Rear panel (spec sheet)

OUTPUT jacks (mono, stereo); Output Level switch L -30 dBm / M -15 dBm / H 0 dBm; PHONES (stereo); VCF CONTROL (FV-200); PEDAL HOLD (DP-2); PATCH SHIFT (DP-2); ARPEGGIO CLOCK input (1 step / pulse, +2.5 V or more); SAVE; LOAD; MEMORY PROTECT; DCB; TUNE (50 cent).

DCB: 14-pin connector, only pins 1 to 7 connected on the Juno-60. Key on/off to and from JSQ-60 / MSQ-700 sequencers and the MD-8 MIDI converter.

## 6. Specifications page

- Keyboard 61 keys, 5 octaves. DCO x6 (Range 16'/8'/4', LFO mod, PWM, PWM mode ENV/MANUAL/LFO, Pulse, Sawtooth, Sub osc + level, Noise). HPF cutoff. VCF cutoff, resonance (0 to self oscillation), ENV mod, key follow 0 to 100 %. VCA selector, VCA level. ENV A 1 ms to 3 s, D 2 ms to 12 s, S 0 to 100 %, R 2 ms to 12 s. LFO rate 0.3 to 20 Hz, trigger AUTO/MAN, delay 0 to 2 s. Chorus I / II.
- Controllers: Volume; Octave Transpose DOWN/NORMAL/UP; LFO trigger; Bend Sens DCO; Bend Sens VCF; Bender; Arpeggio MODE UP / UP & DOWN / DOWN; RANGE 1/2/3; RATE 1.5 to 50 Hz; Arpeggio on/off; Hold; Key Transpose.
- Memory: Patch Number 1 to 8, Bank 1 to 5, Manual, Write, Save / Verify / Load, display. 56 patches.
- Power 30 W. Dimensions 1060 (W) x 378 (D) x 113 (H) mm. Weight 12 kg.
- Options: RH-10, FV-200, DP-2, OP-8, CB-JUNO, KS-2.

## 7. Factory patches (56), from the Roland patch sheets as transcribed in dzannotti/junox

Columns: Rate, Delay, Trig(A/M), DCO-LFO, PWM, PWM-mode(L/M/E), Pulse, Saw, Sub on, Sub lvl, Noise, HPF, VCF Freq, Res, Pol(N/I), Env, LFO, Kybd, VCA(E/G), VCA lvl, A, D, S, R, Chorus(0/1/2). Slider values 0 to 10.

```
11 Strings I            6,0,A, 0,0,L, 0,1,0,0,0, 0, 7,0,N,0,0,10,   E,0,   4,0,10,4.5, 1
12 Strings II           4,0,A, 0,6,L, 1,1,0,0,0, 0, 7,0,N,0,0,10,   E,-2,  4,0,10,4.5, 2
13 Strings III          3,8,M, 0,7,L, 1,1,1,10,0,0, 5,0,N,0,0,10,   E,-2,  3,0,10,6,   2
14 Organ I              2,8,A, 0,5,M, 1,0,1,10,0,0, 4,6,N,4.5,0,10, G,0,   0,0,0,0,    1
15 Organ II             5,4,A, 0,5.5,L,1,0,1,8,0, 0, 3.5,5.5,N,4,0,10,G,0, 0,1,0,1,    1
16 Organ III            5,4,A, 0,5.5,L,1,0,1,8,0, 0, 3.5,5.5,N,3.5,0,10,G,0,0,1,0,1,   2
17 Brass                5,6.5,A,1.5,0,M,0,1,0,0,0,0, 0,0,N,8.5,0,4,  E,2,   2.5,4,6,2, 1
18 Phase Brass          6,0,A, 0,10,E,1,1,0,10,0,0, 3,1,N,5.5,0,10,  G,-1,  2,4,4,3,   1
21 Piano I              6,3,M, 4.5,6,M,1,0,0,0,0, 0, 1,0,N,7,0,4,    E,2,   0,8,1.5,3, 0
22 Piano II             4,0,A, 0,4,M, 1,0,1,4.5,0,0, 3.5,0,N,2.5,2,8,E,3,   0,7.5,0,3.5,0
23 Celesta              3.5,6,A,0,5,E, 1,1,0,10,0,1, 3.5,8,N,0,0,10, E,1,   0,6.5,2,5.5,0
24 Mellow Piano         5,0,M, 0,5,M, 1,0,0,10,0,0, 3,0,N,2.5,1,9,   E,2,   1,7.5,2,8.5,1
25 Harpsichord I        5,4,A, 0,3,M, 1,0,1,7,0, 1, 3,0,N,5,0,7,     E,-1,  0,6,3.5,2.5,1
26 Harpsicord II        5.5,6,A,0,2,M, 1,0,1,8.5,0,1, 5,2.5,N,3,0,10,E,0,   0,5,1.5,5, 2
27 Guitar               6,6,A, 0,6,M, 1,0,0,10,0,2, 3,0,N,4.5,1.5,5, E,4,   0,5.5,3.5,6.5,0
28 Synthetiser Harp     3,8,M, 0,0,M, 0,1,0,10,0,0, 3,0,N,5,0,8,     E,1,   0,5.5,3,5, 1
31 Bass I               5,6,A, 0,5,M, 1,1,1,3,0, 0, 3,2.5,N,3.5,0,0, G,0,   0,4,1,2.5, 1
32 Bass II              5,6,A, 0,5,M, 1,1,0,3,0, 0, 3,5,N,4.5,0,5,   G,-1,  0,3,3.5,2.5,1
33 Clavichord I         6,2.5,M,4,9,M, 1,0,0,0,0, 0, 0,3,N,8,0,6,    E,2,   0,5,3.5,1.5,1
34 Clavichord II        1,0,A, 0,8,M, 1,0,0,10,0,1, 5.5,7,N,2,2.5,7, E,5,   0,4.5,2,2, 0
35 Pizzicato Sound I    5,6,A, 0,3.5,M,1,0,0,3,0, 0, 4.5,4,N,3,3,10, E,3,   0,2,3.5,5.5,1
36 Pizzicato Sound II   5,6,A, 0,2,M, 1,0,1,3,0, 0, 5,4,N,3,0,10,    E,1,   0,3,3,4,   2
37 Xylophone            5,0,A, 0,5,M, 0,0,1,10,0,1, 4,5,N,3,0,6,     E,5,   0,3.5,0,3.5,0
38 Glockenspiel         5,0,A, 0,0,M, 1,0,0,0,0, 1, 4.5,5,N,3,0,6,   E,4,   0,3,2.5,5, 0
41 Violin               6,6,A, 2,0,L, 0,1,0,0,0, 1, 6.5,0,N,0,0,10,  E,2,   4,0,10,4,  0
42 Trumpet              2.5,6.5,A,1.5,0,M,0,1,0,0,0,0, 0,0,N,8.5,0,4,E,2,   2.5,4,6,2, 0
43 Horn                 2.5,7,A,0,0,M, 0,1,0,0,0, 0, 2,0,N,5.5,2,4,  E,2,   4,5,6,3,   0
44 Tube                 2.5,7,A,1.5,0,M,0,1,0,0,0,0, 1.5,0,N,6,0,4,  E,5,   3,4,4,3,   0
45 Flute                5.5,5,A,0,0,M, 0,1,0,0,1.5,1,5,0,N,0,2,6,    E,5,   2,6,5,2.5, 0
46 Clarinet             5,6.5,A,1.5,0,M,1,0,0,0,0,1, 5,3,N,2.5,0,6,  E,1,   2.5,6,6,2.5,0
47 Oboe                 5.5,6.5,A,1.5,6.5,M,1,0,0,0,0,3,4.5,5,N,2.5,0,5,E,5,2,6,6,2.5, 0
48 English Horn         5,7,A, 2,6.5,M,1,0,0,0,0, 3, 5,7,N,0,1.5,5,  E,5,   2,6,6,2.5, 0
51 Funny Cat            6,2,M, 3,0,M, 0,1,0,0,0, 1, 1.5,7.5,N,5,2,5, E,3,   2.5,4,10,1,0
52 Wah Brass            6,2,M, 3,0,M, 0,1,0,0,0, 0, 3,7,N,4.5,0,6,   G,2,   3,3,4,2,   0
53 Phase Combination    6,2,M, 0,8,M, 1,1,0,0,0, 0, 6,2,N,3,0,2,     E,-2,  0,7,2,2,   1
54 Reed I               6,2,M, 4,0,M, 1,0,1,0,0, 0, 1,6,N,7,0,5,     G,1,   0,8.5,5,1, 1
55 Popcorn              0,0,A, 0,0,M, 0,0,1,10,0,1, 2.5,2,N,5.5,0,10,E,3,   0,3,2,0,   0
56 Reed II              3,8,M, 0,0,M, 0,0,1,10,0,0, 2,0,N,6,0,8,     E,0,   0,5.5,3,6, 1
57 Reed III             6,2,M, 2,5,M, 0,0,1,10,0,0, 3,2,N,3,0,10,    E,1,   2.5,0,10,2,0
58 PWM Chorus           3,0,M, 0,5,L, 1,0,1,10,0,0, 8,0,N,0,0,10,    E,-3,  3,0,10,4,  2
61 Synthetiser Organ    4.5,6,A,0,6.5,M,1,0,1,7.5,0,0,2.5,0,N,5,2,7, G,0,   0,2,5,2.5, 2
62 Effect Sound         4.5,6,A,1.5,10,M,1,1,1,7,0,0, 6.5,0,I,4.5,0,7,E,2,  0,5,0,5.5, 1
63 Effect Sound II      5.5,9,A,0,3,L, 0,1,1,6.5,0,0, 6.5,3,I,4,0,1, G,1,   6.5,5.5,2,6.5,1
64 Space Harm           5.5,0,A,2,0,E, 0,1,0,0,0, 0, 6.5,5,N,5.5,0,10,E,1,  0,8,8,9,   1
65 Funk                 3,2.5,A,0,6,M, 1,1,1,10,0,0, 7.5,6,I,5,0,4.5,G,-3,  6,5,0,0,   1
66 Space Sound I        6,7,A, 2,4.5,M,1,0,1,10,0,0, 6.5,7,I,5.5,0,10,G,-2, 0,8,0,3,   1
67 Mysterious Invention 6,8,A, 2,8,E, 1,1,0,10,0,0, 8,7,I,6,2.5,0,   E,0,   0,10,0,10, 0
68 Space Sound II       3,3,A, 0,6,M, 1,1,0,8,0, 0, 2,8.5,N,6,0,10,  E,-3,  10,10,10,10,1
71 Percussive Sound I   0,0,A, 0,0,E, 0,0,0,0,4, 1, 4,10,N,1.5,0,10, E,5,   0,3,0,4,   0
72 Percussive Sound II  0,0,A, 0,0,E, 0,0,0,0,5, 1, 5,10,I,3.5,0,10, E,5,   0,3,0,4,   0
73 Whistle              5.5,5,A,0,0,M, 0,0,0,0,3.5,1,3.5,10,N,1.5,2,10,E,3, 3,0,10,1,  0
74 Effect Sound III     5.5,4,A,0,0,E, 0,0,0,0,3.5,1,3.5,10,N,0,2,10,E,5,   0,4,5.5,7, 0
75 UFO                  6,0,A, 0,0,M, 0,0,0,0,0, 0, 0,10,N,7,4,10,   E,-1,  0,6,10,8,  1
76 Space Sound III      6,0,A, 0,0,M, 0,0,0,0,5, 0, 5,10,I,4,0,10,   E,0,   0,10,0,8,  1
77 Surf                 0,0,A, 0,0,E, 0,0,0,10,6,0, 6,0,N,0,6,10,    E,4,   0,4,10,8,  0
78 Synthetiser Drums    0,0,M, 0,0,E, 0,0,0,0,2, 0, 2,10,N,4,2,10,   E,1,   0,5,0,6,   0
```

Spelling as on the sheets. Bank 7 patches rely on VCF self-oscillation and noise. Whether these may ship in an open-source product is a licensing question (see the plan); they are at minimum the acceptance-test set.

## 8. Hardware behaviours not in the manual

- Chorus I+II via both buttons.
- Hidden banks 8/9 (test programs).
- Mono mode via power-on with KEY TRANSPOSE held + ARPEGGIO MODE up.
- HPF position 0 flat.
- Decay time independent of sustain level.

## 9. Checklist for whoever can open the PDF

1. Exact printed legends: TRANSPOSE (DOWN/NORMAL/UP vs L/M/H), VCA LEVEL scale, VCF polarity glyphs, HPF ticks, waveform glyphs, how banks 6/7 are indicated.
2. Display content in MANUAL mode and at power-up.
3. LFO MAN: does the button gate PWM-by-LFO; does the delay restart per press; does AUTO reset LFO phase.
4. KEY TRANSPOSE direction and interaction with octave TRANSPOSE.
5. Voice stealing rule for releasing vs held voices; HOLD with a 7th key.
6. Whether the manual documents Chorus I+II.
7. Envelope: release during attack; bender VCF depth.
8. MEMORY PROTECT switch form.
9. VCF CONTROL pedal: adds to or replaces FREQ, and range.

## Sources

Roland "Juno-60 Technical Specifications" (support.roland.com, Sweetwater copy); owner's manual passages via search snippets; Service Notes values via Michi71/PicoVintageSynthCollection; measurements from github.com/pendragon-andyh/Juno60; factory patches from github.com/dzannotti/junox patches/Juno60.csv; Matrixsynth "Juno-60 Extra Patch Tricks"; kidnepro and hyperreal tape procedure transcriptions.

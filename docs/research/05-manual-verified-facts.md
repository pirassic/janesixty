# Facts verified first-hand from the Juno-60 Owner's Manual and Service Notes

Read in full on 2026-10-04: Owner's Manual (Roland, '83 APR, 38 pages) and Service Notes (First Edition, April 10 1983, 26 pages). This document supersedes any conflicting statement in research/01 and research/02. Page references: OM = Owner's Manual, SN = Service Notes.

## 1. Corrections to the earlier research notes

1. **There is no 16' / 8' / 4' RANGE switch on the Juno-60.** The DCO section has: LFO slider, PWM slider, PWM mode switch (LFO / MAN / ENV), pulse, sawtooth and sub buttons with LEDs, SUB OSC level slider, NOISE level slider (OM p.12, p.32). Octave selection is the OCTAVE TRANSPOSE switch on the bender panel (DOWN / NORMAL / UP).
2. **OCTAVE TRANSPOSE is stored in the patch.** OM p.7: "The settings of the knobs and the position of the OCTAVE TRANSPOSE switch are memorized as a patch program." OM p.18: "How this switch is set can be written into memory." The factory chart's first column is the transpose position (N / D / U). The switch is read through SW LATCH IC9 on Panel Board B as 2 bits (L / M / H) (SN p.14, p.16 Table 4).
3. **Edit indicator is both decimal points.** OM p.7: "the two dots in the Program Number Display window will light."
4. **Chorus I+II is a stored state.** Test program patch 98 lists CHORUS = 3 (SN p.22 Table 6), so the chorus field takes values 0 to 3 and both buttons on is memorised. The owner's manual documents only OFF / I / II.
5. **Panel legend for the octave switch is DOWN / NORMAL / UP** (OM p.18 figure, p.32 spec). The firmware and the factory chart call the positions L / M / H and D / N / U.

## 2. Panel behaviour, now confirmed

### Memory (OM p.6 to 8)
- 56 patches, 7 banks x 8. Bank buttons are labelled "1(6) 2(7) 3 4 5". Banks 6 and 7: hold bank 5 and press 1 or 2.
- Either bank or patch button may be pressed first; pressing only one changes only that digit.
- Editing: moving any control makes that control live and lights both decimal points. Editing never alters memory.
- Write: MANUAL (or select and edit), MEMORY PROTECT to OFF, hold WRITE, press BANK then PATCH NUMBER, protect back to ON. Display shows "_ _" while WRITE is held before a destination is chosen; the figure also shows "Pr", which is the display when WRITE is pressed with protect on (inferred from the figure, OM p.8).
- Copy: select source, hold WRITE, press destination bank and patch (OM p.8).
- MEMORY PROTECT is a three-position slide switch marked ON OFF ON with OFF in the centre (OM p.11 figure).
- Battery backup, 3 V lithium CR-1/3N (SN p.26).

### Tape (OM p.9 to 11)
- SAVE: display goes blank, pilot tone for 4 to 5 s, then modulated data; about 50 s; on completion the LED goes out and the display shows "- -" (manual mode). MANUAL button aborts.
- VERIFY and LOAD: press before the modulated tone; display blank during; "- -" on success; "Er" on error. LOAD needs MEMORY PROTECT OFF.
- Format (SN p.17): about 340 baud; 0 = four cycles of 1.36 kHz, 1 = seven cycles of 2.38 kHz, 2.94 ms per bit.

### DCO (OM p.12 to 13)
- No portamento ("because of its digitally controlled system").
- Pulse width: MAN = slider sets width, minimum 50 % square at slider 0; LFO or ENV = slider sets depth.
- Harmonics table on OM p.13 shows pulse at 33 % lacking the 3rd, 6th, 9th harmonics; the panel pictogram for PWM.

### HPF (OM p.14)
- "In its lowest position, the DCO output passes the filter unchanged." Position 0 is flat. Four detents.

### VCF (OM p.14 to 15)
- Resonance at maximum self-oscillates; the manual warns the self-oscillation pitch is not accurate to the keyboard and may be unstable until stored.
- Polarity switch pictograms: upright envelope = normal (N), inverted = inverted (I).
- KYBD is "usually set to the maximum on such a long keyboard".
- Rear VCF CONTROL pedal raises the cutoff when pressed (adds). Jack board (SN p.10): 0 to 5 V with a 20 k pedal, 3 V with the wiper at centre or the jack closed, so the pedal jack contributes a fixed 3 V offset when nothing is plugged in.

### VCA (OM p.16)
- GATE mode: VCA controlled by the gate; ENV still drives VCF.
- LEVEL: "adjusts the depth of the ENV modulation", used to match patch loudness; "when set too high, a sound distortion might occur, but this is not because of the trouble of the Juno-60." Scale on the panel figure runs with 0 in the centre; factory chart uses -3 to +5.

### ENV (OM p.16 to 17)
- Spec: A 1 ms to 3 s, D 2 ms to 12 s, S 0 to 100 %, R 2 ms to 12 s.
- "When the sustain level is high, the envelope curve does not change by adjusting the decay time" (decay runs to the sustain level; with S at 10 decay is invisible).
- Diagram shows attack slider 10 reaching max at 3 s, decay slider 10 at 12 s, with the note that the knob positions in the figure are not exact.
- "When all of the ADSR sliders are set to zero, the waveform will be an extremely short pulse wave, and only a short click is heard."

### LFO (OM p.17), the trigger table verbatim
| | AUTO | MANUAL |
|---|---|---|
| Delay 0 | LFO always functions | LFO works while the LFO TRIGGER button is pressed, stops on release |
| Delay > 0 | LFO does not start until the delay time has passed | While the button is pressed the LFO amplitude becomes larger; when the delay time has passed it is at normal amplitude |

"This delay function works only in non-legato manner. So the delay time affects only the first key in a legato section." So in AUTO the delay restarts on the first key of a phrase; in MANUAL the button press starts the delay ramp. Whether the triangle phase resets is still not stated.

### Keyboard, controllers (OM p.18)
- 61 keys; with the transpose switch it behaves as a 7-octave keyboard. In NORMAL the third C from the bottom is middle C.
- Bender lever: DCO and VCF depth sliders. LFO TRIG button works only with TRIGGER MODE at MANUAL.

### Arpeggio (OM p.19, p.23)
- MODE UP / UP&DOWN / DOWN, RANGE 1 / 2 / 3, RATE 1.5 to 50 Hz, external clock in overrides the rate slider (one step per pulse).
- Plays only while keys are held unless HOLD is on. Press the chord keys at the same moment or the first pattern is imperfect. Turn ARPEGGIO on before pressing keys. If the range exceeds the keyboard, the highest octave is repeated. In DOWN mode the first octave may take a while to settle in; switching to DOWN from another mode, the last note of the previous mode becomes the first note of DOWN.
- Pattern table on OM p.19 for a 3-note chord: UP climbs through the octaves; UP/DOWN climbs and descends without repeating the turnaround notes (from the notation); DOWN descends.

### Hold (OM p.20)
- Keeps the gate on; level is the sustain level, so a patch with S = 0 cannot be held. Up to 6 keys; more than 6 played, the last six remain.
- Arpeggio and hold: arpeggio continues after release; a new key pressed starts a new pattern. Turn ARPEGGIO on before HOLD.

### Key transpose (OM p.20)
- Hold the button, press any key in any octave; indicator lights; the keyboard plays in that key. Hold and press any C except the highest to return. "Normally C cannot be transposed; only the highest C can be transposed one octave up." So the shift is upward within the octave (0 to 11 semitones), and the top C gives +12.
- Not possible while an arpeggio is playing. A held chord can be transposed.

### Rear panel (OM p.21, p.33)
- PATCH SHIFT pedal steps 1 to 8 within the bank and wraps.
- DCB: "Please do not connect the Juno-60 to any other device but the OP-8" (manual era note).
- Output level L -30 dBm, M -15 dBm, H 0 dBm. Tune +-50 cents ("+-1/4 note").

### Chorus (OM p.22)
- OFF / I / II; "II is stronger than I"; use the stereo outputs.

## 3. Circuit facts from the Service Notes

### Block diagram (SN p.3)
Per voice x6: DCO (IC54, IC55 are the two 8253s), WAVEFORM, VCF (IC2, 5, 8, 11, 14, 17 = IR3109), VCA (IC19 to 25 = BA662), ENV (IC26 to 31 = IR3R01), S/H for KCV (IC33 to 35) and PWM (IC41, 43, 45). Shared: master osc TR58 to 63, KCV D/A IC51 to 53 (7-bit) through anti-log amp IC38/39, KCV demux IC36, PWM demux IC40, VCF CV demux IC47, ENV mux IC32. Chorus board: HPF IC5, VCA IC6 (uPC1252H2, the patch LEVEL VCA), chorus IC1 to 4. Panel Board A: LFO IC11/12, delay IC15, PWM mode selector IC18 with VCA IC19, noise TR5, VCF CV mix IC20 with KYBD IC21, LFO IC23 and ENV IC24 VCAs (BA662). Panel Board B: patch CPU, RAM, D/A, pot MPX, chorus LFO IC1 to 3, cassette interface, DCB 8251A.

### Master oscillator and counters (SN p.14)
- LC oscillator with varicap D18, control voltages from BENDER, LFO, TUNE, common to all voices. Variable range: BENDER +-700 cents, LFO +-300 cents, TUNE +-50 cents; summed maximum +-1050 cents, 1 MHz to 3.5 MHz, centre about 1.9 MHz.
- 8253: "Assume that the master oscillator runs at 1 902 810 Hz and a divisor of 4305, the counter develops 442 Hz rectangular signals." Divisor data delivered per key from PROM as 8 bit x 2. Counter mode 3 (square wave generator) per the IC data sheet page (SN p.21).

### Sawtooth generator (SN p.14)
- "Analog voltages (a series of 6 key control voltages for 6 channels) from D/A converter will change in 0.48 V/oct steps as different keys are played. KCVs are combined with voltages from TUNE, LFO and BENDER if any, and are fed to anti-log amplifier TR56. The summed voltage increases or decreases in 1 V/oct steps at TR56 output, which is passed on to one of S/Hs (IC33 to IC35) selected by analog demultiplexer IC36. C7 charges (current) in proportion to CV coming on inverting input pin of IC16 and discharges through TR5 at the rate of square wave generated from programmable counter (IC54 or IC55), maintaining the sawtooth amplitude constant over the frequency range."
- So the amplitude-compensation CV is exponential in pitch, 7-bit DAC per key, and includes tune, LFO and bender. Saw swings 0 V to -12 V (Fig. 2).
- Adjustment (SN p.23): VR38 at C2 and VR37 at C7 for 12 Vp-p at TP3; other keys 12 V +-1 V.

### Pulse (SN p.14, p.24)
- Comparator/S&H: PWM CV -6.5 V gives 50 %, -0.5 V gives 97 %. Calibration: PWM 50 % VR6 and PWM 95 % VR7 on Panel Board A; other channels 47 to 50 %. PWM source LFO / MANUAL / ENV selected by IC18 (4051) with BA662 IC19 as depth VCA; the ENV input is multiplexed.

### Sub oscillator (SN p.15)
- 8253 square into D flip-flop (IC3, 9, 15 = HD14013B), divided by two, to TR4; amplitude set by SUB VR20 on Panel Board A.

### Noise (SN p.23)
- TR5 2SC945 "selected for noise" on Panel Board A; NOISE LEVEL VR14 set for 4 Vp-p at TP4 (voice VCA output) with noise at 10.

### VCF control voltages (SN p.15 Fig. 4)
- Per voice: cutoff from VCF CV demux IC47 and S/H. The common VCF CV MIX (IC20 summing) takes: multiplexed KYBD CV (IC51 to 53 D/A) through IC24 BA662 with KYBD depth; LFO through IC21 BA662 with LFO depth; ENV through IC23 BA662 with ENV depth and polarity; CUTOFF FREQ CV; jack VCF CONTROL; bender VCF.
- Calibration (SN p.24): patch 84 (FREQ 3, RES 10, ENV 0, LFO 0, KYBD 0). 8-1 RESONANCE VR1 per channel: 4 Vp-p self-oscillation at TP4. 8-2 FREQUENCY VR2 per channel: 248 Hz (B3) at TP4. 8-3 KYBD OFFSET VR11: with KYBD 10 at C4, 248 Hz, i.e. no shift at C4. 8-4 KYBD GAIN VR10: C6 gives 992 Hz, so key follow at 10 is 1 octave per octave. 8-5 WIDTH VR3 per channel: C6 gives 992 Hz on each voice (expo converter scale match). 8-6 VCF LFO OFFSET VR9 and 8-8 VCF ENV OFFSET VR13: null control feedthrough. 8-7 VCF LFO GAIN VR8: 6 Vp-p at VR3 hot with LFO TRIG; at the output "the frequency should vary between 40 to 50 Hz and 4 to 5 kHz" for patch 87 (FREQ 3.5, RES 10, LFO 10), so LFO full depth spans about 6.6 octaves peak to peak, +-3.3 octaves. 8-9 VCF ENV GAIN VR12: patch 93 (FREQ 0, RES 10, ENV 10) peaks at about 30 kHz (T = 34 us).

### VCA (SN p.23)
- 5-1 VCA GAIN VR4 per channel: 4 Vp-p saw at TP4 for the C4 key. 5-2 VCA OFFSET VR5 per channel: minimise the pulses at TP4 while playing (control feedthrough), patch 83 (ADSR all zero).

### Envelope (SN p.25)
- IR3R01 x6. ENV TIME VR6 of CH1: 3 s attack at TP6 (Fig. 38, exponential rise). Then align the other five channels' rise times to CH1 (Fig. 39). "Check 6 sounds for synchronization in A, D, S and R phases."
- Owner's manual Fig. on p.17: attack top = 3 s, decay/release top = 12 s.

### LFO (SN p.23 to 24)
- 7-1 RATE VR1: 45 ms period (22 Hz) at slider top. 7-2 DCO LFO GAIN VR2: 14 Vp-p at TP19. 7-3 DCO LFO OFFSET VR3: with TRIG MODE at MAN, triangle centred on ground. 7-4 DELAY VR4: AUTO, DELAY 10, waveform disappears on key press and begins to reappear 2 s later (Fig. 31 shows silence then a rising envelope).
- Factory modification SN 265500 up: IC2 reconnection on Panel Board B to decrease LFO amplitude (chorus LFO overmodulation); separately "IC2 Reconnection: decrease LFO amplitude to eliminate overmodulation" (SN p.6).

### HPF and patch VCA (SN p.11 chorus board)
- HPF on the chorus board: HD14051B IC5 selects among C29 0.022 uF, C28 0.01 uF, C27 0.0047 uF with a 1 M x3 and 33 k network, input coupling C47 10 uF; "B A: 01 = 0, 00 = 1, 10 = 2, 11 = 3" are the switch codes. Then VCA IC6 uPC1252H2 with VCA LEVEL CONT from Panel Board A, then IC7 (M5218L) buffer into the chorus input and the direct (dry) path.

### Chorus (SN p.11, p.7)
- IC2 and IC4 MN3009, IC1 and IC3 MN3101 clock drivers with discrete current-controlled oscillators (TR1 to 5, TR9 to 13, 150 pF, 6.8 k, 2.2 k, 1.8 k, 8.2 k, 10 k, 22 k). Bias trimmers VR1 and VR2 (10 kB) set for no clipping with a 6 Vp-p 1 kHz sine at TP8 (SN p.25).
- Pre-filter: TR19/TR18 emitter-follower Sallen-Key stages with 22 k x2, 820 pF / 680 pF and 0.0018 / 270 pF, 10 k series, 0.0022 at the BBD input. Post: 3.3 k x2 sum, 0.0022, 22 k stages 820 pF / 680 pF and 0.0018 / 270 pF, TR7/TR8 and TR15/TR16 (2SK30A) mute, 47 k into IC8 (TA75558) summers with 39 k dry and 100 k / 47 k feedback network, 1.5 k output, TR23/TR22 2SC2878 mute drivers. CHORUS OFF line from Panel Board B: 1 = OFF, 0 = ON.
- Chorus LFO on Panel Board B (IC1 to 3 TA75558, TR1 to 10) with mode switching from the CHORUS I and II lines; the schematic annotates the triangle output as about 20 Vp-p with a 2 s period for I, 20 Vp-p with a 1.25 s period for II, and a smaller amplitude with a much shorter period for I+II (values hard to read on the scan; measured elsewhere as 0.513 / 0.863 / 9.75 Hz). "Larger LFO output causes overmodulation in the chorus circuit ... clock leakage output in CHORUS modes" with a factory modification from SN 265500 (R7 to 30 k, 270 k from +15 V to pin 3).
- Two outputs OUT1 and OUT2 go to the bender board volume pot, then the jack board.

### Jack board (SN p.10)
- Output: IC1 M5218L; level switch L / M / H via a resistor divider (-30 / -15 / 0 dBm); PHONES through 220 ohm; TUNE pot 10 kB.

### Bender board (SN p.13)
- BENDER +ADJ VR2 and -ADJ VR1 (100 kB); DCO sens VR4 10 kA, VCF sens VR5 50 kB, VOLUME VR3 10 kB x2; OCTAVE TRANSPOSE switch via diodes to P16/P17; LFO TRIG switch.
- Bender calibration (SN p.23): patch 82, DCO = 10, lever full left with E5 pressed reads 442 Hz; lever full right with D4 reads 442 Hz: +-7 semitones at full.

### CPU, scanning, assignment (SN p.14 to 17)
- CPU board uPD8049C-238 or -380 (-380 fixes the lowest C with transpose L and long release). Panel Board B uPD80C49C-028.
- Key assignment: "Six channels are assigned to the keys played in the order CH1 to CH6, in the cyclic manner, that is, when the 7th key is played while previously played 6 keys are still held, the 7th key steals the first voice."
- Test modes (power on holding KEY TRANSPOSE 2 s, select with ARPEGGIO MODE): UP = UNISON (six voices on the same key); UP&DOWN = ROTARY (cyclic 1, 2, ... 6, 1; remembers the last channel even after release, new assignment starts with the next channel; the first key does not always activate CH1); DOWN = NON-ROTARY (the channel with the smallest number takes priority for the next assignment; with 4 keys held then 3 released including the first, the next key gets CH1).
- Patch test programs: plug in PATCH SHIFT, banks 8 and 9 reachable with bank 5 + 3 (and + 4), see Table 6 (SN p.22) for the panel settings.
- Panel Board B program loop: 7.2 ms (display refresh timing, SN p.17). All 16 sliders are read by an 8-bit successive-approximation ADC (about 350 us per pot, 5.6 ms for 8 pots, 16 pots per loop) and regenerated by an R-2R DAC through demultiplexers (about 750 us for all 16). So every slider is quantised to 256 steps and refreshed every program loop.
- Switch data: 14 bits (Table 4): VCA env/gate, HPF 2 bits, PWM mode 2 bits, ENV polarity, transpose 2 bits (L / M / H), LFO trigger, chorus II, sub, saw, pulse, chorus I.
- RAM: 2 x 1024 x 4 (1 K byte).
- DCB (SN p.17 to 19): 8251A USART, 31.25 kbaud, 8 data bits, odd parity, 2 stop bits, LSB first. Block = identifier 0FEH (key code) + 6 channel bytes (bit 7 gate, bits 0 to 6 key number) + optional end mark 0FFH. Key number table: C2 = 24, C4 = 48 (middle C, 262.8 Hz), A4 = 57 (442 Hz), C7 = 84, C8 = 96 (4205 Hz). The Juno-60 ignores the patch code 0FDH. More than six channels: the receiver steals the voice whose gate turned on first. External and internal keyboards are parallel; a note already on from one is not retriggered by the other.

### Parts of modelling interest (SN p.25 to 26)
- IR3109 VCF, IR3R01 ADSR, BA662A VCA, uPC1252H2 (patch level VCA), MN3009, MN3101, 2SC945 selected for noise, KV1226 varicap, 37 uH coil, slider pots EVA-TOHC14 50 kA, 50 kB, 1 MA, 10 kA; rotary 10 kB x2 (volume), 10 kB (tune).

## 4. Factory patches: octave transpose column (OM p.25 to 28)

Values per patch number, N = NORMAL, D = DOWN, U = UP:

```
11 N 12 N 13 N 14 N 15 N 16 U 17 N 18 N
21 N 22 U 23 N 24 N 25 U 26 U 27 N 28 N
31 D 32 D 33 D 34 D 35 N 36 U 37 U 38 U
41 N 42 N 43 N 44 D 45 U 46 N 47 N 48 D
51 N 52 N 53 N 54 N 55 U 56 U 57 U 58 N
61 N 62 U 63 N 64 N 65 N 66 N 67 N 68 N
71 U 72 N 73 U 74 N 75 U 76 U 77 N 78 N
```

The remaining columns in research/02 section 7 match the printed chart on a spot check of rows 11, 16, 23, 31, 58, 65, 71 and 78. Factory names on OM p.24 use "Synthesizer" (Synthesizer Harp, Synthesizer Organ, Synthesizer Drum), "Violine", "Harpsichord 2", "Wah Brass", "Space Harp". The chart on p.24 is the authoritative name list.

## 5. Still open after reading both manuals

- Display content in MANUAL mode at power-up: "- -" is shown after tape operations "(Manual mode)", so "- -" is the manual-mode display. Power-up display not stated.
- LFO triangle phase reset on trigger: not stated.
- Whether PWM-by-LFO is gated by the LFO TRIG button in MANUAL: the LFO itself "works while the button is pressed", so all LFO destinations stop. Accept this reading.
- Voice stealing preference between releasing and held voices: the notes say only "steals the first voice" (oldest). Rotary mode ignores release state.
- Chorus LFO exact amplitudes and periods per mode on the schematic annotation (low scan resolution). Measured values stand.
- HPF resistor values on the chorus board (1 M x3 and 33 k are visible; the exact RC per position needs the schematic at higher resolution or a measurement).

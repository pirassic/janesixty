# 30-minute capture protocol for Juno-60 owners

Purpose: let anyone with a Juno-60, a laptop and an audio interface contribute reference recordings. No test gear, no opening the unit. Several units give both the design's behaviour and the unit-to-unit spread.

## Setup
- Mono OUTPUT L into a line input, level switch H, VOLUME at 3 o'clock, record 24-bit at 96 kHz (192 kHz if available; chorus clock residue sits above 20 kHz). No chorus unless stated. Note the serial number, the ROM version if known, and whether the unit has been serviced.
- First record 10 s of silence (noise floor), then a 1 kHz tone from any source through the same input at a known level if available (not required).
- Hold each note for 4 s, release, wait 4 s. Play C3, C4, C5 unless stated (C4 = the third C from the bottom in NORMAL).

## Recordings
1. **Test patch 82 (sawtooth, VCF fully open):** MANUAL mode, panel per Service Notes Table 6 row 82: all LFO/DCO mod 0, saw only, HPF 1 (position 1), FREQ 10, RES 0, ENV 0, LFO 0, KYBD 0, VCA ENV level 10, A 0 D 0 S 10 R 0, chorus off, OCTAVE NORMAL. Record every C and every F from C2 to C7 (12 notes), then repeat with OCTAVE DOWN and UP for C4 only.
2. **Pulse:** as 1 but pulse only, PWM MAN with the slider at 0, 2.5, 5, 7.5, 10, note C4.
3. **Sub and noise:** sub only with SUB OSC at 10 (C2, C4, C6); noise only with NOISE at 10 (10 s, no key needed with VCA GATE? no: hold any key).
4. **Self-oscillation:** all waveforms off, RES 10, KYBD 0, ENV 0, LFO 0: FREQ at 0, 1, 2, ... 10 (hold any key 3 s each). Then KYBD 10 with FREQ 3: play C2, C4, C6.
5. **Filter sweep:** saw only, RES 0 then RES 5 then RES 10: FREQ at 0, 2, 4, 6, 8, 10, note C3.
6. **ENV to VCF:** saw, FREQ 0, RES 0, ENV 10 then 5, A 0 D 10 S 0 R 0, polarity N then I, note C3.
7. **LFO to VCF:** saw, FREQ 5, RES 5, LFO 10 then 5, LFO RATE 2.5, note C3, hold 8 s.
8. **Envelopes:** saw, filter open. A at 0, 2.5, 5, 7.5, 10 (D 0 S 10 R 0). D at 2.5, 5, 7.5, 10 (A 0 S 0 R 0). R at 2.5, 5, 7.5, 10 (A 0 D 0 S 10). S at 2.5, 5, 7.5 (A 0 D 5 R 0). One note each, C4.
9. **LFO:** saw, DCO LFO 5, LFO RATE at 0, 2.5, 5, 7.5, 10 (4 s each); then RATE 5 with DELAY TIME at 2.5, 5, 7.5, 10, AUTO mode, play C4 after 3 s of silence each time.
10. **Chorus:** record OUTPUT L and R as a stereo pair. Saw, filter open, C4 held 8 s with chorus OFF, I, II, and I+II (both buttons). Then the same with all waveforms off, RES 10, FREQ 5 (a sine) for the delay sweep.
11. **HPF:** noise only at 10, HPF at 0, 1, 2, 3, 5 s each.
12. **Gate mode:** saw, VCA GATE, C4, three short stabs.
13. **Factory patches:** 11, 12, 17, 21, 31, 41, 51, 58, 64, 75 at C3 and C4, 4 s each, with the patch's own chorus setting.

## Naming
`<serial>_<item>_<setting>_<note>.wav`, one zip per unit, plus a text note with the serial, date, room temperature if known, and anything that seemed off (a voice dropping out, drift).

## What it gives the project
Items 1 to 3: DCO amplitude staircase, pulse law, sub and noise levels. 4 to 7: the expo converter law, resonance onset, depth curves. 8 to 9: envelope and LFO tables. 10: chorus rates, delay range, filter corners, noise. 11: HPF corners. 13: acceptance references for the preset bank.

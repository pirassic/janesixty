# Calibration report

Status of every constant the DSP reads, by source. Updated at each phase 4 step; the CI job `sim-reference` uploads the latest simulation-vs-plugin comparison (`sim/ir3109-report.md`) as an artifact on every push.

Source tags (from `calibration/juno60.json`): `service-notes`, `owners-manual`, `schematic`, `published-measurement`, `simulation`, `plugin-derived`, `assumed`.

## Phase 4 step 1 (2026-10-05): IR3109 and chorus board

### Reference simulation
- `tools/sim/ir3109_ref.py` runs ngspice on a behavioural IR3109 (four OTA-C stages, tanh cells with the 68 k / 560 input divider) in two forms: the plugin's loop sum (`model`, with the 0.308 input-side compensation) and the Service Notes p.9 resistor network as drawn (`network`, BA662 as a transconductance, both polarities).
- `tools/sim/Ir3109Dump.cpp` runs the plugin block through the same measurements; `tools/sim/compare.py` writes the report.
- Measured: small-signal response at FC 1 kHz; self-oscillation frequency and level at the plugin's corner and kMax; harmonics of a 12 Vp-p sine; passband gain vs resonance.
- Known before the first CI run: the plugin block's self-oscillation level and frequency depend on the host sample rate (the 248 Hz / 4 Vp-p fit holds at 48 kHz). The continuous-time reference decides the rate-independent value and the fit moves there in the next step.

### Chorus board (from the Service Notes, this step)
| Item | Value | Source |
|---|---|---|
| Output summer dry leg | 39 k into 100 k feedback (IC8, R89 / R88, R95 / R91) | schematic p.11 |
| Output summer wet leg | 47 k into the same feedback, so wet = 0.83 x dry at the summer | schematic p.11 |
| BBD path gain before the summer | +2.3 dB | published-measurement (Holters and Parker) |
| BBD overload point | 6 Vp-p 1 kHz at TP8 (chorus input) with VCA LEVEL approx. 0 and CHORUS I, bias set so TP1 is not clipped | service-notes p.25 |
| Overload shape | linear to the knee, tanh over 1.5 V | assumed |

### FREQ slider slope (this step, from the demo recording)
`VcfMapping::octavesPerSliderUnit` 0.9 -> 1.33, source: the 13.3-octave cutoff span in the Juno-6 and Juno-106 specifications over 10 slider units, anchored at the measured 248 Hz at FREQ 3 (Service Notes 8-2, patch 84 with KYBD 0; KYBD pivot C4 per 8-3, 1 oct/oct per 8-4). Confirmed against the factory demo recording, see research/10. Implied ENV full depth 10.9 octaves (published 10.8 to 11).

### Resonance network result (CI run 37308210767)
With the BA662 as a differential transconductance and the IR3109 output buffered, the p.9 network as read (R5 47 k into pin 3, R3 100 k into pin 2, R1 / R2 1.5 k, R14 10 k, R7 68 k / R6 560) gives:

| normalised k | 0 | 1 | 2 | 3 | 3.9 |
|---|---|---|---|---|---|
| network, passband at 100 Hz (dB) | -1.4 | +3.6 | +4.7 | +5.2 | +5.4 |
| plugin model, 0.308 compensation (dB) | -0.2 | -3.6 | -5.3 | -6.3 | -6.9 |

Oscillation threshold gm 32.6 mA/V (the resistor arithmetic predicts 31). Self-oscillation at k 4.05 and a 270 Hz corner: 251.9 Hz and 3.52 Vp-p against the plugin's 248.0 Hz and 4.05 Vp-p; the level gap is the BA662's own tanh limiting in the feedback leg, which the plugin does not model.

The passband goes the wrong way: the drawn network gains 6.8 dB with resonance (equivalent coefficient about 2.5), where owners describe resonance thinning the sound and the plugin-derived loss is about 7 dB. R5 = 470 k instead of 47 k would give a coefficient of 0.25 and a loss of 7.9 dB, so the resistor value read from the scan is the prime suspect. `vcf.qCompensation` stays 0.308, plugin-derived, until R5 is confirmed on a more legible scan or the Juno-6 schematic.

### Compensation settled by the recording (2026-10-05)
R5 confirmed 47 k by the owner on two scans, so the resistor is not the discrepancy. Discriminator: in the demo recording the gain is fixed across patches, so the loudness of resonant patches relative to non-resonant ones is a direct measure of the passband gain at resonance. Plugin renders in the demo's register, loudness of sounding frames, plugin minus recording, with the RES 0 to 1 patches as the zero:

| qCompensation | RES 1.5 to 4 (n 10) | RES 4.5 to 7 (n 15) | RES 7.5 to 10 (n 10) |
|---|---|---|---|
| 0.308 (current) | -1.5 dB | -1.7 dB | -9.7 dB |
| 1.0 (constant passband) | +2.6 | +3.5 | -7.3 |
| 2.4 (network as drawn, ideal BA662) | +7.2 | +8.0 | -5.2 |

The hardware loses passband with resonance about as the 0.308 model does (the last column is bank 7's self-oscillating effect patches, whose levels the demo clearly rode). The ideal symmetric-input BA662 in the netlist is therefore the wrong element; the chip's two inputs do not weigh equally, which no datasheet can confirm. `vcf.qCompensation` stays 0.308, now tagged `plugin-derived, confirmed by the demo recording to about 2 dB`.

### VCF trim offset (this step)
`vcf.trimOffsetOct` = 1.0, `assumed` (unit condition). The demo recording's cutoffs sit a constant ~1.2 octaves above the Service Notes trim across the whole FREQ range (per-patch best offset against the recording: +1.25 oct at FREQ 0 to 4, +0.64 at 4 to 6, +1.25 at 6 to 8, many patches at the +2 limit of the sweep). The manual's own numbers stay as the anchor (248 Hz at FREQ 3); the offset is the first entry of the condition layer and belongs in a user setting (Service Notes trim / demo unit) in a later step.

### Open questions carried
- BA662 input asymmetry: the drawn network with an ideal OTA predicts a passband rise that the recording rules out. A transistor-level BA662 model, or a bench sweep, would close it.
- Resonance network, resolved on a second reading (owner's crop and an external findings note, 2026-10-05): the 47 k compensation leg enters BA662 pin 3 (+) with R2 1.5 k to ground, the 100 k feedback leg enters pin 2 (-) with R1 1.5 k. Input adds, feedback subtracts. The `network` rows in the simulation report now run this differential form on a resonance axis normalised by its own simulated oscillation threshold, so the drawn compensation's passband curve can be read against the plugin's 0.308 coefficient. That coefficient stays `plugin-derived`: the ~7 dB loss it was fitted to has no primary measurement behind it.
- The external findings note also quotes R4 = 12 k and R7 / R11 = 33 k; the schematic shows R14 10 k and 68 k stage resistors, so its derived 0.37 coefficient is not used.
- Voice mixer resistor values (saw / pulse / sub / noise legs) still unread; the mixer ratios remain plugin-derived.

## Assumed values (phase 4 exit list, 2026-10-05)
Every entry in `calibration/juno60.json` whose source starts with `assumed` (`Calibration::assumedKeys()`), with its uncertainty and what it touches. vcf.qCompensation, chorus.dryGain and chorus.wetGain were assumed earlier and now carry schematic sources.

| key | value | uncertainty | effect if wrong | how to close |
|---|---|---|---|---|
| `vcf.trimOffsetOct` | 1.0 oct | +-0.3 oct (per-patch best offsets 0.64 to 1.25, several at the sweep limit) | every cutoff shifts; brightness of the whole bank | user setting done (Settings, "Match the factory demo unit", default on; stored in plugin state); a bench sweep of a trimmed unit would settle the manual side |
| `env.timingCapNf` | 47 nF | +-50 % | none: loaded but unused; the ADSR time tables are measured and the capacitor would only matter for a component-level envelope | read it off schematic p.9 or drop the key |
| `chorus.bbdClipVpp` (knee; source is a test condition, not `assumed`) and `chorus.bbdClipRoomV` | 6 Vp-p, 1.5 V | knee +-1.5 V, room +-1 V | overload shape of the wet path on six-voice chords above LEVEL +3; silent below (chords sit 10 dB under the knee at LEVEL 0) | MN3009 input THD sweep at the bias point, or Holters & Parker's measured curve |
| `voicing.lowShelfDb` | +1.0 dB | +-1.5 dB (the recording's sub band agrees within the method's noise with it in, listening notes 2026-10-05) | low end 150 Hz down | none from the demo; a line recording of a known unit, or zero it as a user voicing |
| `voicing.lowShelfHz` | 150 Hz | +-50 Hz | as above | as above |

Assumptions that live in code rather than the file (comments say `assumed`), none pinned by a source:

| where | value | effect |
|---|---|---|
| `VcfMapping::selfOscSliderPos` (Voice.h) | RES 7.8 reaches k = 4 | where self-oscillation starts on the slider; bank 7 patches |
| bender to VCF (Voice.h) | +-2 oct at full | bender VCF depth only |
| noise bleed at NOISE 0 (Voice.h) | -100 dB | lets self-oscillation start; inaudible |
| manual PW law, sub and noise slider tapers, mixer ratios (Voice.h, `plugin-derived`) | raised cosine, two-segment, -1.3 / -1.8 dB | mixer balance; the resistor legs on p.9 would replace them |
| arpeggio clock and gate (Synth.cpp) | 1.5 to 50 Hz log, 0.55 gate | arpeggio only |
| chorus mute fade, BBD hiss (ChorusBoard.h) | 150 ms, -72 dB re 4 Vp-p | switch feel; noise floor |

`plugin-derived` sources (kept apart from `assumed`): vcf.qCompensation 0.308 (confirmed by the recording's loudness balance within 2 dB), env.sustainLevel law, the mixer ratios above.

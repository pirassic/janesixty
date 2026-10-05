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

### Open questions carried
- The p.9 resonance network read literally (R5 47 k from the mixer and R3 100 k from the output into the same BA662 input, R1 1.5 k to ground) gives an input-to-feedback ratio of about 2.1, far from the 0.308 compensation that reproduces the known ~7 dB passband loss. Either the drawing hides an inversion or the mixer-side tap is not the mixer output. The `network` rows in the simulation report show what the drawn network does.
- Voice mixer resistor values (saw / pulse / sub / noise legs) still unread; the mixer ratios remain plugin-derived.

## Assumed values
Every entry in `calibration/juno60.json` whose source starts with `assumed` (`Calibration::assumedKeys()`): vcf.qCompensation, env.timingCapNf, chorus.dryGain and wetGain were assumed until this step and now carry schematic sources; chorus.bbdClipRoomV, voicing.lowShelfDb and voicing.lowShelfHz remain assumed.

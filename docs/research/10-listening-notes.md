# Listening notes against the factory demo recordings

Reference: the owner's comparison of the plugin with a recording of all 56 factory patches (YouTube, compressed, unknown signal chain). These are ear judgements, not measurements, and every change made from them is tagged `assumed` or "listening fit" so the phase 4 simulation and any future bench capture can overrule it.

## 2026-10-05, build after commit e6af9f5

Observations: the original sounds fatter and roughly 10 to 15 % heavier in the bass; the organ and celesta patches are more percussive; the top end is more open; the VCF seems to sit higher at the same slider positions.

Changes and reasoning:

| Observation | Change | Grounding |
|---|---|---|
| VCF sits higher, highs more open | Tried FREQ slider slope 1.0 octave per unit; **reverted to 0.9** the same day. At 1.0 the top of the slider reached 31 kHz, far above the hardware's ~20 kHz, and the owner heard FREQ 8 to 10 as noise. | The anchor is measured (Service Notes adj. 8-2); the slope is not, but 0.9 is the only value that puts FREQ 10 near the hardware's open-filter corner. |
| Highs closed | IR3109 2x oversampling path: input held for both sub-steps, output through a 31-tap half-band decimator (flat to 15 kHz, -0.6 dB at 20 kHz). The earlier interpolate-then-average pair cost about 1.9 dB at 10 kHz and 3.3 dB at 15 kHz. The corner is also capped at 0.22 of the oversampled rate, where the solver is still accurate. | Numerical artefact, no circuit meaning. |
| (found while chasing the noise) alias tones at -40 dB on every note | The DCO applied its PolyBLEP residual at twice the correct weight since phase 1, so it cancelled nothing. Fixed; aliases now sit below -62 dB in the filtered output. | Bug, no circuit meaning. |
| Organ and celesta less percussive | VCF ENV and VCF LFO depth sliders now linear (`VcfMapping::cvDepth`) instead of square-law | The depth sliders feed BA662 control VCAs (IC23, IC24 on Panel Board A), whose gain is linear in control current. The square law was copied from another plugin. Bender and DCO LFO depth keep the square law until checked. |
| Bass 10 to 15 % light | Provisional output low shelf, +1.0 dB below 150 Hz, after the chorus (`voicing.lowShelfDb` in `calibration/juno60.json`) | No circuit source. The Juno-60 HPF at 0 is flat per the Owner's Manual; the bass-boost position is a Juno-106 feature. A 1 dB difference is also inside what a YouTube chain can do to a recording, so this is a voicing fudge, kept in the calibration file so it can be zeroed. |
| (side effect) chords clipped after the ENV change | Master gain 0.08 -> 0.045 and a soft safety stage: linear to -1 dBFS, tanh above, never exceeding full scale. A first version with the knee at -3 dBFS was audible on Mellow Piano at LEVEL +3 with chorus off, which the hardware would play clean. | Safety only, not a component. The hardware's overload path is the chorus board (no compander), to be modelled before the chorus in phase 4. |

Open questions for phase 4 (ngspice) or a bench capture: FREQ slider slope, ENV depth law, the chorus board's low-frequency response (the only plausible circuit source for the bass difference), the output-stage clipping level.

## 2026-10-05, after the safety-stage build

Owner: high VCA LEVEL soft-clips; is that consistent with the original? Answer: the stage heard was the plugin's safety limiter, not a component. The original's overload path is the chorus board's BBD input (no compander), pinned by the Service Notes bias procedure at 6 Vp-p (LEVEL 0) at the chorus input. That path is now modelled in its place (wet path only, before the BBD, `chorus.bbdClipVpp`), and the summer gains follow the schematic (wet 0.83 of dry) instead of the Hera fit, which lowers the wet level by about 5.5 dB relative to the previous build. The safety stage stays at -1 dBFS.

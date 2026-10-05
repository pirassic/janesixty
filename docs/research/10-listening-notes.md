# Listening notes against the factory demo recordings

Reference: the owner's comparison of the plugin with a recording of all 56 factory patches (YouTube, compressed, unknown signal chain). These are ear judgements, not measurements, and every change made from them is tagged `assumed` or "listening fit" so the phase 4 simulation and any future bench capture can overrule it.

## 2026-10-05, build after commit e6af9f5

Observations: the original sounds fatter and roughly 10 to 15 % heavier in the bass; the organ and celesta patches are more percussive; the top end is more open; the VCF seems to sit higher at the same slider positions.

Changes and reasoning:

| Observation | Change | Grounding |
|---|---|---|
| VCF sits higher, highs more open | FREQ slider slope 0.9 -> 1.0 octave per unit (`VcfMapping::octavesPerSliderUnit`), range now ~31 Hz to ~31 kHz around the fixed 248 Hz at FREQ 3 anchor | The anchor is measured (Service Notes adj. 8-2); the slope was never measured. 1 oct/unit is the simplest law that spans the audible range. |
| Highs closed | IR3109 2x oversampling path: input held for both sub-steps, second sub-step is the output. The earlier interpolate-then-average pair cost about 1.9 dB at 10 kHz and 3.3 dB at 15 kHz. | Numerical artefact, no circuit meaning. A half-band pair is still planned for phase 4. |
| Organ and celesta less percussive | VCF ENV and VCF LFO depth sliders now linear (`VcfMapping::cvDepth`) instead of square-law | The depth sliders feed BA662 control VCAs (IC23, IC24 on Panel Board A), whose gain is linear in control current. The square law was copied from another plugin. Bender and DCO LFO depth keep the square law until checked. |
| Bass 10 to 15 % light | Provisional output low shelf, +1.0 dB below 150 Hz, after the chorus (`voicing.lowShelfDb` in `calibration/juno60.json`) | No circuit source. The Juno-60 HPF at 0 is flat per the Owner's Manual; the bass-boost position is a Juno-106 feature. A 1 dB difference is also inside what a YouTube chain can do to a recording, so this is a voicing fudge, kept in the calibration file so it can be zeroed. |
| (side effect) chords clipped after the ENV change | Master gain 0.08 -> 0.065 and a soft output stage: linear to -3 dBFS, tanh above, never exceeding full scale | The real output op-amp runs out of rail too; the knee is assumed. |

Open questions for phase 4 (ngspice) or a bench capture: FREQ slider slope, ENV depth law, the chorus board's low-frequency response (the only plausible circuit source for the bass difference), the output-stage clipping level.

# Settings

One **Settings** menu. In the Standalone it is the button at the top left of the window's title bar, which also holds the audio and MIDI setup, saving and loading the state, and the reset; in a plugin host it is the Settings button at the right of the preset strip.

The unit settings are the condition layer: properties of the unit being emulated, as opposed to the patch. They are saved with the plugin instance (the project, or the Standalone's saved state) and never written into presets, so a preset sounds the same relative to its panel whatever the settings. Each one is a documented uncertainty of the model (`docs/calibration-report.md`), not a sound-design control.

| setting | choices | default | what it does |
|---|---|---|---|
| Match the factory demo unit | on / off | on | Applies the VCF trim of the unit in Roland's factory demo recording, one octave above the Service Notes trim. Off restores the manual's trim: self-oscillation at 248 Hz with FREQ 3 and RES 10, every patch one octave darker. |
| Voice spread | off / Service Notes tolerances / twice | Service Notes tolerances | Fixed per-voice deviations: saw level +-1 V of 12 (Service Notes spec), cutoff +-2 %, resonance +-2 %, envelope times +-8 %, VCA +-0.5 dB. Off gives six identical voices. |
| Filter drive | -6 dB / as calibrated / +6 dB | as calibrated | Level into the IR3109's OTA pairs, output compensated, so only the filter's saturation changes. "As calibrated" is the schematic's mixer and summing-node reading (page 9); the demo recording cannot tell the three apart, so the range is there for taste and for units whose mixer parts drifted. |
| Chorus noise | off / as calibrated / aged BBDs (+10 dB) | as calibrated | The MN3009 hiss (no compander). Aged BBDs are reported noisier. |


## Extras

The hardware has no velocity and no MPE. Both are opt-in, off by default, and when off the plugin's output is bit-identical to the plain instrument. A badge ("VEL", "MPE") appears in the preset strip while either is on, because they are not part of the patch.

| setting | choices | default | what it does |
|---|---|---|---|
| Velocity | off / to VCA level / to VCF ENV depth / to both; amount 50 % or 100 %; soft curve | off | A note's velocity scales the destination: at 100 % a velocity of 0.3 gives 0.3 of the level (or of the envelope depth); at 50 % it gives 0.65. The soft curve takes the square root first. |
| MPE | on / off; pressure off / to VCA / to VCF ENV / to both | off | MPE lower zone: channel 1 is the master (its bend is the bender, as always), channels 2 to 16 carry one note each. A member channel's pitch bend bends only that note (48 semitones at full), and its channel pressure drives the chosen destination with 50 % amount. |

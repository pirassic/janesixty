# Settings

One **Settings** menu. In the Standalone it is the button at the top left of the window's title bar, which also holds the audio and MIDI setup, saving and loading the state, and the reset; in a plugin host it is the Settings button at the right of the preset strip.

The unit settings are the condition layer: properties of the unit being emulated, as opposed to the patch. They are saved with the plugin instance (the project, or the Standalone's saved state) and never written into presets, so a preset sounds the same relative to its panel whatever the settings. Each one is a documented uncertainty of the model (`docs/calibration-report.md`), not a sound-design control.

| setting | choices | default | what it does |
|---|---|---|---|
| Match the factory demo unit | on / off | on | Applies the VCF trim of the unit in Roland's factory demo recording, one octave above the Service Notes trim. Off restores the manual's trim: self-oscillation at 248 Hz with FREQ 3 and RES 10, every patch one octave darker. |
| Voice spread | off / Service Notes tolerances / twice | Service Notes tolerances | Fixed per-voice deviations: saw level +-1 V of 12 (Service Notes spec), cutoff +-2 %, resonance +-2 %, envelope times +-8 %, VCA +-0.5 dB. Off gives six identical voices. |
| Filter drive | -6 dB / as calibrated / +6 dB | as calibrated | Level into the IR3109's OTA pairs, output compensated, so only the filter's saturation changes. "As calibrated" is the schematic's mixer and summing-node reading (page 9); the demo recording cannot tell the three apart, so the range is there for taste and for units whose mixer parts drifted. |
| Chorus noise | off / as calibrated / aged BBDs (+10 dB) | as calibrated | The MN3009 hiss (no compander). Aged BBDs are reported noisier. |

Planned for the same menu: velocity and MPE (phase 5).

# Settings

The **Settings** button at the right of the preset strip opens the condition layer: properties of the unit being emulated, as opposed to the patch. They are saved with the plugin instance (the project, or the Standalone's saved state) and never written into presets, so a preset sounds the same relative to its panel whatever the settings.

| setting | default | what it does |
|---|---|---|
| Match the factory demo unit | on | Applies the VCF trim of the unit in Roland's factory demo recording, one octave above the Service Notes trim (`vcf.trimOffsetOct` in the calibration file). Off restores the manual's trim: self-oscillation at 248 Hz with FREQ 3 and RES 10, and every patch one octave darker. |

Planned for the same panel: voice spread and drive (phase 4), velocity and MPE (phase 5).

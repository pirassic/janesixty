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

## 2026-10-05, measured comparison against the demo recording

The owner supplied the demo's audio (128 kbps MP3) with chapter times. `tools/listen/pitches.py` estimates each segment's register, `RenderPatches` plays every factory patch in that register through the full DSP chain, and `compare_reference.py` compares band balance (sub, low, mid, hi-mid, high), spectral tilt, stereo width and attack time per patch.

Finding: with the FREQ slider at 0.9 octaves per unit the plugin was dark on nearly every patch. Third-octave spectra of Strings 1 (FREQ 7, no ENV) show the recording as a plain 6 dB/octave sawtooth out to 14 kHz, i.e. the real filter is effectively open at FREQ 7, where the plugin rolled off at 24 dB/octave from about 2.5 kHz. Mean difference (plugin minus reference) over 56 patches:

| slope (oct/unit) | mid | hi-mid | high | tilt (dB/oct) |
|---|---|---|---|---|
| 0.9 | -6.1 | -10.3 | -19.5 | -3.4 |
| 1.2 | -5.3 | -7.1 | -11.8 | -1.6 |
| 1.33 | -4.8 | -6.3 | -9.6 | -1.0 |
| 1.5 | -4.4 | -5.3 | -7.2 | -0.4 |

Patches with FREQ above 5 come within 2 to 4 dB at 1.33; the residual sits in the FREQ 3 to 5 group (about -8 dB in the mids), which no slope fixes with the anchor pinned, so it points at the mixer levels (sub, pulse) or the PWM law rather than the filter. Adopted 1.33: the spec-derived span, consistent with the ENV depth. The earlier 1.0 trial had been judged with the DCO aliasing bug present, which is why it was heard as noise at the top of the slider.

## 2026-10-05, resonance compensation against the recording

Relative loudness of resonant patches (RES 1.5 to 7) against non-resonant ones matches the plugin's 0.308 compensation within 2 dB; the schematic-derived 2.4 would make them 7 to 8 dB too loud. Kept 0.308. See the calibration report.

## 2026-10-05, overload on organ, celesta, piano at moderate LEVEL

Owner: distortion that LEVEL drives and VOLUME cannot remove. Cause: the BBD overload point (6 Vp-p at TP8) was right, but the plugin fed the chorus board the raw voice sum, eleven times hotter than the hardware, which sums voices at 0.122 each and divides by 0.75 again before TP8 (schematic p.9). Both gains are now modelled; the master gain is raised to keep the output level. Chords now sit about 10 dB below the BBD knee at LEVEL 0 and only LEVEL +5 with six voices reaches it, which matches owners' reports of the chorus gritting up only on very hot patches.

## 2026-10-05, "more sparkle" on the video: where it came from

Owner, after the summer fix: the video has more sparkle, the plugin is thinner, attack slightly more aggressive. Eliminated in order, each by rendering the bank and comparing: the chorus wet filters (tripling their corners moved the top band 2 dB), the chorus itself (forcing it off made the chorus patches darker still; the stereo side channel shows the wet path at the recording's level), key tracking below C4 (halving it barely moved the dark group). Found: a uniform PolyBLEP droop in the top octave (fixed by running the DCO at 2x) and, after that, a constant offset: each patch's best global cutoff shift against the recording is about +1.2 octaves whatever its FREQ setting, with or without ENV. The Service Notes are self-consistent at FREQ 3 (248 Hz self-oscillation; the 8-7 LFO sweep test centres on 450 Hz at FREQ 3.5), so the unit in the video is trimmed about an octave above the manual. Added `vcf.trimOffsetOct` (condition layer): 1.0 matches the recording, 0 is the manual's trim.

Result after the 2x DCO and the trim offset, mean over 56 patches (plugin minus recording): mid -2.6 dB (was -6.0 in the morning), hi-mid -1.5 (was -10.3), high -1.5 (was -19.5), tilt +1.0 dB/oct (was -3.4), sub -6.6 (was -4.7). The top end is closed; the recording keeps about 6 dB more below 150 Hz, which is the next item (sub oscillator level or the video's chain).

## 2026-10-05, Mellow Piano distorts with chorus I at LEVEL +2 and VOLUME 10 (Standalone, CI run 32)

Owner: expected? Measured through the full chain (2 s held chord, peak): a six-note Mellow Piano chord at the factory LEVEL +2 reaches 1.96 V peak at TP8, 4 dB under the BBD knee, so the modelled overload is idle; with chorus I the dry plus wet sum enters the output safety stage at 1.47 (knee 0.9) and 22 % of the samples sit in its tanh. A C3 triad stays clean (0.80). Cause: the master gain 0.49 put digital full scale at about 1.8 V at the chorus summer, below the modelled BBD knee, so the plugin's limiter fired before the hardware's overload path would. The hardware summer runs on 15 V rails with VOLUME a pot after it, so that chord is clean on the original. Change: master gain 0.49 to 0.22 (about 7 dB), chosen so the safety stage only engages once the chorus input is past 4 V, where the BBD model is already gritting; VOLUME default 8 to 10 to keep the Standalone's level usable. Not a calibrated value; nothing in the volt-domain chain depends on it.

## 2026-10-05, the 6 dB sub band deficit was the demo's left-hand bass line

Open item 1 from the handover. Per patch, the sub band (40 to 150 Hz, plugin minus recording) split cleanly by register, not by the sub oscillator: patches whose fundamental or sub oscillator sits in the band (Bass 1, Bass 2, Tuba, Organ 2, Space Sound 1) matched within 1 dB; sub-oscillator patches in register averaged -1.2 dB; the deficit was carried by patches played far above the band (Harpsichord 1 -23, Guitar -25, Funny Cat -25, Space Harp -33, Celesta -11). The recording's silent gaps are 40 dB below that content, so it is not a floor. Bandpassed to 40 to 150 Hz, Harpsichord 1 and Guitar hold steady pitches for whole bars (82, 88, 98, 104, 110, 124, 131, 146 Hz), and Celesta one D3: the player accompanies the melody with a bass line that the dominant-pitch estimate in `pitches.py` cannot see behind the melody. `pitches.py` now reads the bass register from the sub band separately and adds the note under the triad and alone. With it the sub mean goes from -6.6 to +1.4 dB (median +2.0, n = 48 sane patches), with the bass note rendered at full melody level, so the plugin's low end sits inside the method's resolution of the recording. No model change. `voicing.lowShelfDb` (+1 dB, assumed) is neither confirmed nor refuted by this: the in-register patches read -1.2 to +0.9 with it in, inside the noise. Kept at the owner's ear. The bass detector is fooled by onset thumps on Glockenspiel (false bass note, +28 dB outlier); ignore that row.

## 2026-10-05, attack (open item 2): nothing measurable on the synth side

Rise times (20 dB, median per patch) on ATTACK 0 ENV-mode patches: recording 26 ms median, plugin 5 ms; GATE mode 31 against 18 ms. The plugin is already faster everywhere. A 128 kbps MP3 round trip of the plugin renders leaves the rise times unchanged (4 ms stays 4 ms), so the recording's slow rises are the performance (rolled chords, staggered notes), not the codec. Onset brightness (2 to 12 kHz over 200 Hz to 2 kHz in 0-12, 12-35, 35-100, 100-300 ms windows) differs per patch by a near-constant offset across the windows, which is the known per-patch cutoff balance, and the onset-specific part has no consistent sign over 19 patches (Piano 1 plugin brighter at onset by 19 dB, Xylophone recording brighter by 10, median about 0). A note-on click above the closed filter was looked for on Celesta (VCF ENV 0): the top band is flat at the MP3 noise floor through the onset, so no gate or envelope feedthrough is visible in the recording either. Rendering with the minimum attack at 0.3 ms instead of 1 ms changes none of these measures. Conclusion: the recording cannot show a softer attack in the plugin, and the candidates in the handover (gate rise, minimum attack, DCO reset) have nothing to fix against. The likely cause of the owner's impression is the output safety stage: before today's master gain change, chord peaks at VOLUME 8 to 10 were in its tanh region, which flattens exactly the transients. Re-listen with the 0.22 build before any model change.

## 2026-10-05, chorus width (open item 3): about 1 dB once tonal balance is removed

Width per band (side over mid energy within the band, 150-500 / 500-2k / 2-6k / 6-15k Hz), median over the chorus patches, plugin minus recording: mode I (23 patches) -1.4 / -1.3 / -1.1 / -0.1 dB, mode II (7) -0.9 / -1.4 / -1.2 / -0.6 dB. The -3.5 dB broadband width in the handover mixed in the mid channel's own band balance. The residual is a uniform 1.2 dB on the wet level with no frequency shape, inside the method's noise and inside the tolerance of the schematic ratio (39 k / 47 k) and the published +2.3 dB BBD path gain. Left as is. Sensitivity, from two candidate renders: +3 dB on the BBD path gain raises the per-band width by about 1.9 dB (overshooting to +0.5), a 1.5x wider delay sweep changes it by 0.1 dB or less, so the residual would be about +2 dB of wet level, not modulation depth. Note that the demo's 128 kbps joint-stereo MP3 can only have reduced the recording's side channel, so the real unit is at least as wide as measured.

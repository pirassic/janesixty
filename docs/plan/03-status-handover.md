# Jane-Sixty: status and handover (2026-10-05)

Read this first in a new session. It states where the project is, what is settled and why, what is open, and the next steps. Detailed evidence lives in `docs/calibration-report.md`, `docs/research/10-listening-notes.md` and `docs/research/05-manual-verified-facts.md`.

## Identity and ground rules
- Product **Jane-Sixty**, vendor **clevergear**, bundle id `com.clevergear.jane60`, codes `Clvg` / `Jn60`. Identifiers freeze at v1.0; the display vendor can change.
- GPLv3, JUCE 9.0.3 (AGPLv3) as a submodule in `libs/JUCE`, CMake 3.25+, C++20. No Roland or Juno trademark in product name or identifiers (NOTICE, README).
- Branch `claude/nice-wozniak-1s5ih6` (continues `claude/admiring-tesla-vj12gr`); every push runs CI. Reference audio lives on branch `reference-audio` (never merged) and locally under the gitignored `reference/`.
- Owner preferences: ask when readings differ materially, challenge assumptions, no verbosity, never a double hyphen.

## Where the build stands
- Formats: AU, VST3, Standalone, macOS universal (arm64 + x86_64, 11.0+), ad-hoc signed in CI. Artifact `jane60-macos-unsigned` on every green run; install with `xattr -dr com.apple.quarantine`.
- CI jobs: `dsp-tests-linux` (26 Catch2 tests, DSP library only), `plugin-macos` (build, tests, auval, pluginval level 5, artifact), `sim-reference` (ngspice IR3109 reference vs the plugin block, report artifact `sim-reference-report`).
- Phases 0 to 3 done (research, DSP, presets, panel UI). Phase 4 (calibration) in progress and far along. Phases 5 to 7 not started.

## Sound model, current state and provenance
All constants are in `calibration/juno60.json` with a source tag each. Settled this session, in audible order:
1. **DCO**: PolyBLEP at 2x with a 31-tap half-band decimator (`src/dsp/core/HalfBand.h`). Two earlier bugs fixed: the residual was applied at twice its weight (no anti-aliasing at all), and the two-point PolyBLEP drooped 3 to 9 dB in the top octave at the host rate.
2. **IR3109 filter**: four TPT tanh stages, 2x with the same decimator, corner capped at 0.22 of the oversampled rate. The ngspice reference confirms the solver, the 8 % self-oscillation shift and 4 Vp-p at kMax 4.05. FREQ slider **1.33 oct/unit** (spec span) anchored at the Service Notes 248 Hz at FREQ 3, plus **`vcf.trimOffsetOct` 1.0** because the factory demo recording sits a constant octave above the manual's trim (0 restores the manual). Resonance compensation 0.308 confirmed by the recording's loudness balance; the drawn resonance network with an ideal BA662 predicts the opposite sign, which means the BA662's input weighting is the open physics item, not the resistors (R5 47 k confirmed on two scans).
3. **ENV and LFO depth** into the VCF: linear in the slider (BA662 control VCAs); bender and DCO LFO depth still square-law.
4. **Voice summer**: 0.122 per voice (27 k into 3.3 k) and 0.75 to TP8 (schematic p.9). This fixed the "LEVEL distorts, VOLUME can't help, chorus off cures it" complaint.
5. **Chorus board**: summer dry 39 k / wet 47 k into 100 k (wet 0.83 of dry), BBD path +2.3 dB (Holters and Parker), BBD soft overload at the 6 Vp-p bias test point (knee shape assumed), pre and post filter corners in the calibration file (jpcima's schematic-derived set; tripling them changes little).
6. **Output**: provisional +1 dB low shelf below 150 Hz (`voicing`, assumed), safety soft stage at -1 dBFS (not a component).

Measured against the demo recording (tools below), mean over 56 patches, plugin minus recording, with the bass line rendered: sub +1.4 dB (was -6.6 without it), low -1.2, mid -2.8, hi-mid -2.0, high -2.1, tilt +0.5 dB/oct, stereo width -8.1 dB (chorus patches about -3.5). Owner's ear: strings similar, bass better, sparkle now close, attack slightly less aggressive than the video.

## Tools you will use
- `tools/listen/pitches.py` estimates each demo segment's register and writes `reference/phrases.txt`; `tools/listen/RenderPatches.cpp` (`jane60_render_patches`) renders all 56 patches through the full chain in that register; `tools/listen/compare_reference.py` compares band balance, tilt, width and attack per patch and writes `reference/comparison.md`. A full render-and-compare takes about three minutes; experiments are done by editing a copy of the calibration JSON and passing it to the renderer.
- `tools/sim/ir3109_ref.py` (ngspice netlists, model and network forms), `tools/sim/Ir3109Dump.cpp`, `tools/sim/compare.py`; CI runs them.
- Reference audio: fetch branch `reference-audio`, decode with `ffmpeg -i <mp3> -ar 48000 -f f32le reference/ref48k.f32`, the labels are on the same branch (`juno60-reference-audacity-labels.txt`, copy to `reference/labels.txt`); `pip install numpy`; the four hidden patches (Organ 2, Organ 3, Brass, Horn) are split in `compare_reference.segments()`.
- Settings menu: `Jane60Editor::showSettingsMenu` in `src/plugin/PluginEditor.cpp`; state property `demoTrim` in `PluginProcessor.cpp`.
- Panel UI: `src/ui/PanelLayout.h` (reference space 2260 x 850, computed columns), `src/ui/PanelComponent.cpp`, `src/ui/Controls.cpp`. Keyboard shortcuts for auditioning in `docs/user/keyboard-shortcuts.md`.

## Open items, in priority order
1. ~~Sub band~~ closed 2026-10-05: the 6 dB was the demo's left-hand bass line missing from the render phrase, not the model (`pitches.py` now renders it; sub mean +1.4 dB). `voicing.lowShelfDb` stays assumed, inside the noise.
2. ~~Attack~~ closed 2026-10-05 pending the owner's ear: the plugin's rises are faster than the recording's everywhere (5 against 26 ms on ATTACK 0), the recording's rises are the performance (an MP3 round trip does not slow the plugin's), no click above the closed filter on Celesta, and a 0.3 ms minimum attack changes nothing. Likely cause of the impression: the old 0.49 master gain put chord peaks into the safety stage's tanh. Owner re-listens with the 0.22 build.
3. ~~Chorus width~~ closed 2026-10-05: per band the side channel is a uniform 1.2 dB low (mode I -1.4/-1.3/-1.1/-0.1 dB), the -3.5 was the tonal balance leaking into the broadband ratio. Inside the schematic ratio's tolerance; left.
4. **Condition layer / settings**: the trim choice is done (2026-10-05): `Condition` struct in the DSP (`src/dsp/Condition.h`, `Synth::setCondition`), a `demoTrim` bool in the plugin state (not in presets), default on, one Settings menu: the Standalone's title-bar button (JUCE's Options button hidden and replaced from `Jane60Editor::parentHierarchyChanged`), the strip button in hosts (`docs/user/settings.md`). Voice spread and drive still to model and add there. Side observation from the test: self-oscillation grows from the -100 dB noise bleed and takes about 3 s to reach full level on a silent patch; the hardware starts faster (check bank 7 patches by ear).
5. **BA662 input weighting** (research only): a transistor-level model or a bench sweep.
6. Calibration report: list every `assumed` key with its uncertainty (phase 4 exit criterion).

## Next phases (updated)
- **Phase 4 finish**: items 1 to 4 above, then the calibration report exit.
- **Phase 5, extras and Windows**: velocity and MPE as opt-in settings; **Windows Standalone + VST3** (x64, MSVC on `windows-latest` in CI, pluginval, zip artifact; no AU, no AAX). CMake already lists formats per platform (`JANE60_FORMATS`), so the work is a CI job, MSVC warning cleanup (`/W4`, the DSP uses `-Wconversion`-clean code already), and a Windows preset folder (`%APPDATA%\clevergear\Jane-Sixty`, the `PresetManager::userFolder()` path is the one place to change). Expect first-run issues with `juce::FileChooser` and the window default size code using the primary display.
- **Phase 6, release**: signed and notarized macOS `.pkg`, Windows installer (Inno Setup or WiX), user manual, v1.0.
- **Phase 7, performance layer**: scale and chord modes, Circuit Rhythm input, Launchpad LED feedback (research/06).

## How a session should start
1. `git fetch origin claude/nice-wozniak-1s5ih6 && git checkout claude/nice-wozniak-1s5ih6`; `git submodule update --init` only if building the plugin (CI does that; Linux builds the DSP with `-DJANE60_BUILD_PLUGIN=OFF`).
2. `cmake -S . -B build -DJANE60_BUILD_PLUGIN=OFF && cmake --build build -j8 && ctest --test-dir build`.
3. Fetch the reference audio as above if listening comparisons are planned.
4. Push every change; check the CI run; the macOS artifact is what the owner listens to.

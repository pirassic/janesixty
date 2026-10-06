# Jane-Sixty (jnsynth)

Open-source, component-modelled recreation of the Roland Juno-60 as a native macOS AU and VST3 plugin and standalone app, with a planned scale-and-chord performance layer. Vendor: clevergear. Licence: GPLv3 (JUCE under AGPLv3).

Status: phase 4 (calibration) complete; next is phase 5 (velocity and MPE opt-ins, Windows).

## Fidelity

Jane-Sixty is modelled from documents, not from a unit on the bench. What that means, in the order the sound passes through it (`docs/calibration-report.md` has every constant with its source):

- **Set by the Service Notes adjustment procedure**, so a correctly serviced Juno-60 matches to within that procedure's tolerances: master clock and tuning, 12 Vp-p saw, PWM endpoints, 4 Vp-p voice output, the filter's 248 Hz anchor and 4 Vp-p self-oscillation at FREQ 3, key follow, LFO and ENV sweep spans, envelope and LFO endpoints, the chorus bias point.
- **From the schematic**, cross-checked against an ngspice reference in CI: the IR3109 four-pole with its 68 k / 560 R input dividers and 240 pF stages, the resonance network, the voice summer, the HPF, the chorus summers and the pre and post BBD filter chains.
- **From published measurements of real units**: envelope, LFO and chorus delay tables, the BBD path gain.
- **Fitted to Roland's factory demo recording** (the 56 patches, 128 kbps MP3): the FREQ slider slope and the resonance compensation sign. Band balance over the bank sits within about 3 dB of the recording, stereo width within about 1 dB, attack faster than any onset in the recording.
- **Assumed, with an uncertainty, and exposed as user settings where audible**: the drive into the IR3109 (the mixer legs are not legible on the available scan; +-6 dB offered), the BBD hiss level (aged setting offered), per-voice tolerances (voice spread, Service Notes tolerances by default), the unit's VCF trim (the demo unit sits one octave above the manual; both offered), a +1 dB shelf below 150 Hz.
- **Not modelled**: DCO saw flyback shape, supply coupling into the noise, BBD clock residue, the DCO reset transient.

If you can put a Juno-60 on a bench, `docs/research/08-bench-protocol.md` says what to capture; the calibration file is designed to be replaced without touching the DSP.

## Build

```
git clone --recurse-submodules https://github.com/pirassic/jnsynth.git
cd jnsynth
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

Targets: `Jane60_AU`, `Jane60_VST3`, `Jane60_Standalone` (macOS). On Linux, configure with `-DJANE60_BUILD_PLUGIN=OFF` to build the DSP library and tests only, or install `libasound2-dev libx11-dev libxrandr-dev libxcursor-dev libxinerama-dev libxext-dev libxi-dev libfreetype-dev libfontconfig1-dev` for the Standalone and VST3 (compile check only; not a release platform).

Layout: `src/dsp` (framework-free DSP and calibration, no JUCE), `src/plugin` (JUCE processor and editor), `calibration/juno60.json` (every hardware constant with its source), `tests/unit` (Catch2), `docs/` (plan and research).

- `docs/plan/00-overview.md`: scope, decisions taken, fidelity target without hardware, name proposals
- `docs/plan/01-research-plan.md`: remaining research and the document-and-simulation calibration method
- `docs/plan/02-development-plan.md`: stack, architecture, block specs, phases, testing
- `docs/research/`: compiled research notes; `05-manual-verified-facts.md` is authoritative where notes conflict

Roland and Juno are trademarks of Roland Corporation. This project is not affiliated with or endorsed by Roland.

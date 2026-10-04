# Jane-Sixty (jnsynth)

Open-source, component-modelled recreation of the Roland Juno-60 as a native macOS AU and VST3 plugin and standalone app, with a planned scale-and-chord performance layer. Vendor: clevergear. Licence: GPLv3 (JUCE under AGPLv3).

Status: phase 0 (foundations). The plugin loads, validates and makes no sound yet.

## Build

```
git clone --recurse-submodules https://github.com/pirassic/jnsynth.git
cd jnsynth
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

Targets: `Jane60_AU`, `Jane60_VST3`, `Jane60_Standalone` (macOS). On Linux without the JUCE system packages, configure with `-DJANE60_BUILD_PLUGIN=OFF` to build the DSP library and tests only.

Layout: `src/dsp` (framework-free DSP and calibration, no JUCE), `src/plugin` (JUCE processor and editor), `calibration/juno60.json` (every hardware constant with its source), `tests/unit` (Catch2), `docs/` (plan and research).

- `docs/plan/00-overview.md`: scope, decisions taken, fidelity target without hardware, name proposals
- `docs/plan/01-research-plan.md`: remaining research and the document-and-simulation calibration method
- `docs/plan/02-development-plan.md`: stack, architecture, block specs, phases, testing
- `docs/research/`: compiled research notes; `05-manual-verified-facts.md` is authoritative where notes conflict

Roland and Juno are trademarks of Roland Corporation. This project is not affiliated with or endorsed by Roland.

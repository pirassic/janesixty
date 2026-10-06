# Contributing to Jane-Sixty

Thank you for considering it. Jane-Sixty is a component model of the Roland Juno-60, free software under GPLv3, and it gets better through exactly the kinds of help listed here.

## What helps most

1. **A real Juno-60 on a bench, even briefly.** `docs/research/08-bench-protocol.md` lists the captures that would settle the values the model still assumes (filter drive, chorus hiss and overload, per-voice tolerances, noise spectrum). A phone recording is not enough; a line-level capture at 48 kHz or better is.
2. **Listening notes against a real unit** or a known clean recording: which patch, what differs, in words. `docs/research/10-listening-notes.md` shows the form.
3. **Schematic readings.** Several mixer values were settled by someone reading a crop of the Service Notes carefully. If you have a clean scan, crops of specific regions are gold.
4. **Bug reports** with the host, the operating system, the preset and what you did. For sound issues, a short audio example and the settings menu's state.
5. **Code**: DSP, UI, platform support, tests. See below.
6. **Documentation and translations** of the user manual.

## How the project is organised

- `src/dsp` is the instrument, framework-free C++20, tested with Catch2 on Linux without JUCE. `src/plugin` and `src/ui` are the JUCE wrapper and the panel.
- `calibration/juno60.json` holds every hardware constant with a **source tag**: `service-notes`, `schematic`, `datasheet`, `published-measurement`, `plugin-derived` or `assumed`. A change to a number is a change to its source line too. The calibration report (`docs/calibration-report.md`) is the ledger.
- `docs/research` is the evidence; `docs/plan` is the plan and the handover note that each working session leaves for the next.
- `tools/sim` runs an ngspice reference of the filter in CI; `tools/listen` compares renders of the 56 factory patches with the factory demo recording.

## Working on the code

```
git clone --recurse-submodules https://github.com/pirassic/janesixty.git
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build && ctest --test-dir build
```

On Linux configure with `-DJANE60_BUILD_PLUGIN=OFF` to build the DSP and tests only, or install the packages listed in the README for a Standalone and VST3 compile check. CI builds macOS universal (AU, VST3, Standalone), Windows x64 (VST3, Standalone) and runs the tests, pluginval and the simulation comparison on every push.

Guidelines:

- Keep the DSP free of JUCE. Keep the extras-off path bit-identical (there is a test for it).
- Sound changes need evidence: a source for the number, or a measurement, or a comparison against the demo recording with the tools. "It sounds better to me" is a listening note, not a calibration.
- Tests for new behaviour; `-Wall -Wextra -Wconversion` clean on GCC and Clang, `/W4` clean on MSVC.
- Open an issue before anything larger than a fix, so design questions are settled before the work.
- Commit messages say what and why; no ticket-style prefixes needed.

## Licensing

Contributions are accepted under GPLv3, the project's licence. JUCE is used under its AGPLv3 option. Do not add code you cannot license this way, and do not add Roland or Juno trademarks to names or identifiers.

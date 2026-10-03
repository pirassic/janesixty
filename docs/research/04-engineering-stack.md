# Engineering stack: frameworks, AAX, macOS distribution, UI, legal, DSP infra, MIDI

Status: research notes, compiled 2026-10-03. Several primary sources (forum.juce.com, kvraudio, developer.avid.com, tal-software.com, cherryaudio.com) were blocked from the sandbox; facts from those are marked **[partially verified]**. Nothing here is legal advice.

## 1. Frameworks and licensing

### JUCE
- Current: JUCE 9.0.1 (9.0.0 shipped July 2026). 9 adds a new SVG parser, variable fonts, new CoreAudio implementation. 8.0.11 updated VST3 SDK to 3.8.0 and AAX SDK to 2.9.0, added a MIDI 2.0 UMP demo.
- Licence: dual **AGPLv3 or commercial**. JUCE 8 removed the splash screen requirement and the Personal income cap [partially verified]. JUCE 9 made no pricing or EULA changes.
- Open-source implication: a GPLv3 project uses JUCE under AGPLv3 at no cost, no splash.
- Formats: VST3, AU, AUv3, AAX, LV2, Standalone. CLAP via the MIT clap-juce-extensions shim (check JUCE 9 compatibility).
- **AAX SDK is bundled in JUCE** (modules/juce_audio_plugin_client/AAX/SDK) and its LICENSE.txt allows use under GPLv3 as an alternative to Avid's agreement.
- README minimum macOS 10.11.

### iPlug2
zlib-like licence; CLAP, VST2, VST3, AUv2, AUv3, AAX, WAM, standalone; min macOS 10.13. Smaller community, less mature GUI layer for complex skins.

### DPF
ISC; LADSPA, DSSI, LV2, VST2, VST3, CLAP, JACK. No AU, no AAX in mainline. Not viable alone.

### CLAP
MIT. Hosts: Bitwig, REAPER, FL Studio, Studio One (partial). **Not Logic, not Pro Tools, not Cubase.** clap-wrapper (MIT) wraps to VST3/AUv2/AAX/standalone; AAX still needs GPLv3 or Avid licence plus PACE signing. Nice fourth format, not a path to the three targets.

### nih-plug (Rust)
VST3, CLAP, standalone only; no AU or AAX; README says maintenance mode.

### Recommendation
**JUCE 9 under AGPLv3**, project licensed GPLv3, optional CLAP via clap-juce-extensions. Only mature framework with first-class AU + AAX + VST3 + standalone, AAX SDK bundled under GPLv3 terms, mature accessibility and HiDPI, largest body of open-source synths to study. iPlug2 is the runner-up for a permissive licence.

## 2. AAX

- **Pro Tools loads only AAX.** No AU/VST3 in any Pro Tools version. ARA 2 is an extension of AAX, not a route for other formats. Wrappers (Blue Cat PatchWork, Element) are the only way to host non-AAX plugins.
- SDK licence: usable under GPLv3 as redistributed inside JUCE, so an open-source project can legally build AAX.
- **Signing is the real blocker**: release Pro Tools loads only AAX signed with PACE wraptool. Unsigned AAX loads only in the Pro Tools Developer build (register as an Avid developer, email devauth@avid.com). Avid enrols registered developers in PACE's developer plan at no cost for AAX-only signing; a physical iLok USB (~$50 to 60) is required. Whether this is unchanged in 2026 [partially verified].
- Consequences: PACE tooling and credentials are closed and personal; CI cannot sign without the maintainer's iLok; community forks cannot produce working AAX. GPL "installation information" clause is a grey area for signed binaries.
- Peers: Surge XT ships no AAX and says so in its FAQ; Dexed and Odin 2 ship VST3/AU/CLAP only; OB-Xd ships AAX only via discoDSP's commercial builds; Vital's AAX status unclear.
- Practical model: build AAX in CI (compiles under GPLv3), sign and release from the maintainer's machine, label as maintainer-signed, or treat AAX as a stretch goal and document the wrapper route.

## 3. macOS distribution

- Apple Developer Program ($99/yr) for Developer ID Application and Installer certificates. Sign with hardened runtime and timestamp, notarize with notarytool, staple. Plugin bundles need hardened runtime + timestamp; the installer gets notarized. AAX additionally goes through wraptool after Apple codesign.
- Universal binary: CMAKE_OSX_ARCHITECTURES="arm64;x86_64". Intel still relevant through ~2027.
- Deployment target recommendation: **macOS 11**. Logic Pro 12 requires 15.6+, Pro Tools 2026 requires 14.8+.
- AU validation: `auval -v aumu <subtype> <manu>`, also `-strict`. Logic caches results: Plug-in Manager "Full Audio Unit Reset" or delete ~/Library/Caches/AudioUnitCache and `killall -9 AudioComponentRegistrar`.
- pluginval: strictness 5 minimum for host compatibility in CI; 10 nightly (includes real-time safety checks).
- GitHub Actions: hosted macOS runners free for public repos. macos-15 (arm64), macos-15-intel (until Aug 2027), macos-26. Universal builds work on arm64 runners.
- Template: pamplejuce (JUCE 9, CMake, Catch2 3.8, pluginval, universal binary, notarization).
- Distribution: GitHub Releases with notarized .pkg plus zip; Homebrew cask in own tap (audio_unit_plugin, vst3_plugin, aax_plugin stanzas).

## 4. UI approach for a faithful hardware panel

- Vector vs filmstrip: JUCE has no built-in filmstrip component (trivial to write). PNG filmstrips need 2x/3x sets for Retina (OB-Xd uses 128 frames). Vector (Path/Drawable from SVG) scales freely; JUCE 9 SVG parser still lacks filters and blurs, so pre-render shadows or draw them in code. Recommended hybrid: **vector layout + procedural shading** (Path, ColourGradient, DropShadow) for panel, sliders, LEDs, text, with a few bitmap textures at 1x/2x/3x or generated at runtime at the current scale. Avoid setBufferedToImage on vector components at fractional scales.
- Resizable: setResizable(true, true) + setResizeLimits + fixed aspect ratio; draw in a reference coordinate space scaled by AffineTransform; store size in state. TAL-U-NO-LX and Dexed are the models.
- Commercial references: Arturia and Cherry Audio use photographic pre-rendered layers with zoom steps; TAL uses flat vector; Roland Cloud uses a photoreal panel. A faithful-but-clean vector rendition is the most tractable and sidesteps trade-dress copying of textures.
- Fonts: 1980s Roland panels use Helvetica or a variant (Juno-60 face not conclusively identified; likely Helvetica / Helvetica Condensed caps). Open substitutes: Nimbus Sans / Nimbus Sans Narrow (URW, GPL + font exception), TeX Gyre Heros Cn (GUST), Roboto Condensed (Apache/OFL), Archivo Narrow, Liberation Sans Narrow (OFL). Do not embed Helvetica itself.
- Slider caps and LEDs: Path with gradient + highlight + soft shadow; LEDs as radial gradient with glow; LED state = parameter value, never local state.
- Slider feel: LinearVertical, setMouseDragSensitivity, velocity mode, fine mode via Shift/Cmd with re-anchoring, double-click reset, mouse-wheel step; offer absolute "jump to click" and relative drag.
- Accessibility: JUCE >= 6.1 exposes components to VoiceOver by default; set titles, descriptions, AccessibilityRole::slider, keyboard focus order, text readouts.
- Open-source JUCE synths to study: OB-Xd, Dexed, Odin 2, Surge XT (vector skin engine), Monique, Helm, KR-106.

## 5. Trademark and trade dress (facts, not legal advice)

- Roland owns ROLAND and JUNO, filed US applications for JUNO-60 and JUNO-106 (Feb 2019), registered TB-303 and TR-808 visual designs as trademarks in Germany, litigated Behringer over BOSS trade dress, and sells its own JUNO-60 Software Synthesizer. A confusingly similar name or panel competes directly with a Roland product.
- Industry practice: TAL "U-NO-LX", Cherry Audio "DCO-106", Arturia "Jun-6 V"; none use Roland or Juno in the product name; descriptive references with a disclaimer are standard.
- Risks: (a) trademark in name, bundle id, manufacturer code; (b) trade dress: exact panel graphics, logotype, colour-coded layout, wood-grain photo; (c) copying factory patch names and data verbatim (debated); (d) SysEx/DCB are protocols, reading dumps is fine.
- Guidelines: original name; never Roland or Juno in name, bundle id, 4-char codes, repo slug or icon; descriptive mention only, with disclaimer; redraw the panel in own style (keep layout and function, change lettering, logo, textures); no Roland logo or JUNO-60 wordmark; own factory patches or credited community patches; NOTICE file; trademark clearance opinion if the project grows.

## 6. DSP infrastructure

- Oversampling: juce::dsp::Oversampling (2x to 16x, polyphase IIR or FIR half-band); oversample only nonlinear stages (filter, chorus BBD). Alternatives: chowdsp resamplers, HIIR.
- chowdsp_utils: BSD-3 for core, data structures, math, simd, parameters, plugin_state, presets, serialization; GPLv3 for dsp_utils, filters, sources, waveshapers (ADAA), eq, gui, plugin_base. chowdsp_wdf is BSD-3. Jatin's BBD code is GPL-compatible.
- Faust: LGPL 2.1 compiler; generated C++ belongs to the .dsp author. faust2api emits a drop-in C++ class. Good for prototyping; hand-written C++ usually wins for a 6-voice VA.
- Real-time safety: no allocation, locks or logging on the audio thread; ScopedNoDenormals; atomics and lock-free FIFOs; preallocate in prepareToPlay; pluginval level 10; RADSan / -fsanitize=realtime (Clang 20+).
- SIMD: juce::dsp::SIMDRegister (4 lanes); 6 voices as 2 groups of 4 padded to 8, or vectorize across samples. Keep it simple first.
- Filters: juce::dsp::LadderFilter (TPT), chowdsp SVF, or own ZDF OTA cascade with tanh and 2x oversampling.
- Testing: Catch2 3.8 unit tests; golden tests rendering fixed MIDI offline and comparing to committed WAVs with tolerance (RMS, peak, spectral); null tests against reference recordings as manual perceptual checks (exact nulls impossible, use spectral envelope / MFCC distance). Also: state round-trip, parameter ranges, denormal-free on silence, block-size independence, sample-rate independence within tolerance.
- pluginval strictness 5 in CI per format; 10 nightly with --randomise and --repeat.

## 7. MIDI implementation checklist

- Core: note on/off, velocity (Juno-60 had none; optional velocity to VCA/VCF), pitch bend with range, mod wheel to LFO depth, CC64 hold, all-notes-off/reset, channel filter, Juno voice stealing, optional portamento (106).
- CC map: documented default map (table + json), 14-bit CC pairs, NRPN via juce::MidiRPNDetector, MIDI Learn (right-click, per-parameter min/max/invert, stored in state), Program Change + Bank Select to preset slots. VST3 has no raw PC input; JUCE maps via setCurrentProgram; expose programs so PC works in Cubase.
- VST3 caveat: no raw CC; JUCE emulates CCs as hidden parameters (JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS, keep on).
- MPE: not needed; decide early between juce::Synthesiser and MPESynthesiser for the voice manager.
- SysEx import (Juno-106): see research/03 section 1.11. Accept .syx with concatenated 128-patch banks; "receive from hardware" in standalone. Map 106 params onto 60 params.
- MIDI 2.0: JUCE 8+ has UMP and MIDI-CI; Logic 12 shows MIDI 2.0 data; Cubase 13+ "compatible"; Pro Tools none. Optional: accept high-res velocity/CC if delivered as UMP.
- Host automation vs CC: every control is an AudioProcessorParameter in an AudioProcessorValueTreeState; CC/learn write to parameters so automation and CC never fight; switches as AudioParameterChoice; parameter IDs stable forever and never reordered (AAX identifies by index).
- Presets: hosts produce .vstpreset/.aupreset/.tfx from the state blob; also ship a host-agnostic JSON preset format (version, parameter IDs by name, optional MIDI map), built-in browser, factory bank in BinaryData, user folder ~/Library/Audio/Presets/<Vendor>/<Plugin>/; import 106 .syx.
- A/B compare: two state snapshots; toggle copies current edits into the active slot; Copy A to B; A/B state non-automatable but saved; modified asterisk; UndoManager on the APVTS.
- Extras: MIDI panic, activity LED, standalone MIDI-in device selection, optional MIDI out of panel moves.

## Uncertainties to re-check
1. Avid Pro Tools Developer and free PACE enrolment still free in 2026.
2. JUCE 9 minimum macOS deployment target.
3. clap-juce-extensions compatibility with JUCE 9.
4. Vital's AAX status.
5. Roland JUNO-60 / JUNO-106 US trademark registration status and classes.
6. Exact Juno-60 panel typeface.
7. Faust architecture-file licences.

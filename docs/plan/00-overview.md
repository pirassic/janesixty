# jnsynth: project overview and open decisions

Goal: an open-source, component-modelled (not sampled) recreation of the Roland Juno-60, with a Juno-106 mode, as a native macOS AU / VST3 / AAX plugin and standalone app, with a panel that matches the original in look and feel plus modern preset management, A/B compare and a full MIDI implementation.

Companion documents:

- `01-research-plan.md`: what still has to be learned, how, and what source inputs are needed from the owner of a real unit.
- `02-development-plan.md`: architecture, phases, milestones, acceptance criteria.
- `../research/`: the compiled research notes the plans rest on.

## Where things stand after the first research pass

What is already known well enough to start building:

- Full signal flow and the per-voice vs shared split (research/01 section 1).
- DCO principle, counter resolution, reset circuit values, PWM law, sub derivation.
- IR3109 topology and the standard Roland component values (68 k / 560 R / 240 pF), resonance via BA662, input-side Q compensation, 248 Hz calibration anchor, C4 key-follow pivot.
- HPF caps and corners (154 / 339 / 720 Hz, position 0 flat on the 60).
- Envelope timing tables from a real unit, curve shapes, decay independent of sustain.
- LFO rate and delay tables from a real unit.
- Chorus: BBD count, measured LFO rates and delay ranges for I / II / I+II, pre and post filter pole sets from the schematic, summer ratios, mute behaviour.
- Voice assignment rules, arpeggio, hold, key transpose, memory and tape behaviour, the 56 factory patches.
- Juno-106 differences, SysEx format, prior art and the modelling literature per block.
- Engineering stack, AAX constraints, macOS signing, UI approach, trademark practice.

What is not known and cannot be settled from paper (research/01 section 12): absolute levels into the filter, the OTA saturation point, cutoff slider to Hz curve, ENV and LFO depth in octaves, envelope sustain law, saw amplitude staircase, chorus pre-filter corner (6.5 vs 9.7 kHz), BBD noise and clock residue, HPF position 0, firmware timing. These need a bench session on a real, freshly calibrated unit. The research plan specifies exactly which recordings.

Note on sources: the four URLs supplied with the brief (owner's manual, service manual, Anwander, Thea Flowers) were all unreachable from this sandbox's network policy. Thea's article was read from its GitHub source. The two manuals and Anwander's pages were reconstructed from secondary sources that quote them with page and figure references. Both PDFs should be read first-hand by a person before the behaviour spec is frozen; the checklist is in research/02 section 9.

## Decisions that need the owner's call

### 1. Which instrument is the primary target?

The brief names both the Juno-60 and the Juno-106 and supplied Juno-60 manuals. They are not the same instrument: the 106 has software envelopes, a crystal-clocked CPU with quantised vibrato and portamento, a bass-boost HPF position, no arpeggiator, no ENV-PWM, 128 patches and MIDI SysEx. A model that is faithful to one is not faithful to the other.

Recommendation: **Juno-60 first, as the reference model**, because its envelopes and clock are analogue and the available measurements are for the 60. Add a **106 mode** as a second phase that switches the envelope generator to the firmware model, changes the HPF table, LFO range, voice allocation modes, adds portamento and the 128-patch SysEx import. The two share the DCO core, filter, VCA and chorus models with different calibration tables.

### 2. "Identical sounding" to what?

No two Junos sound identical; they drift and age. Three possible targets:

- (a) the design as specified by the Service Notes calibration procedure (a perfectly calibrated unit);
- (b) one specific reference unit, measured;
- (c) (a) plus a "condition" layer of per-voice tolerances that can be dialled from factory-fresh to worn.

Recommendation: **(c)**, with (b) as the acceptance test. Calibrate the reference unit per Service Notes, measure it, fit the model to it, then expose per-voice tolerance as a user control. This is also what Roland's own v2 plugin does.

### 3. "Model the original components exactly"

A real-time SPICE simulation of six voices plus chorus is not feasible on a laptop CPU, and nobody ships one. What is feasible and is the state of the art:

- white-box models derived from the schematic (nodal DK or wave digital filters for the chorus board and HPF; a nonlinear zero-delay-feedback OTA cascade for the IR3109 with per-stage tanh; a time-domain integrator model for the DCO with the exact 8253 integer period);
- offline SPICE or ACME.jl simulation of each circuit as a reference to validate the real-time model against;
- component values taken from the schematic, with the few unknowns (OTA bias currents, BBD transfer loss, DAC staircase) fitted to bench measurements.

This is what the plan proposes. It is "component modelling" in the honest sense; it is not a transistor-level simulation in real time.

### 4. AAX and Pro Tools

An open-source project can legally build AAX (the SDK inside JUCE is usable under GPLv3), but Pro Tools only loads AAX binaries signed with PACE tooling tied to the maintainer's iLok. CI cannot sign; forks cannot ship AAX. Surge XT, Dexed and Odin 2 all decided not to ship AAX for this reason.

Recommendation: build AAX in CI from day one so it never rots, register as an Avid developer, and sign releases from the maintainer's machine. Document it as "maintainer-signed". AU and VST3 cover Logic and Cubase with no such constraint.

### 5. Look and feel vs trade dress

The brief says the look and feel must match the original. Roland has registered or applied for JUNO-60 and JUNO-106 as marks, registered the visual design of other instruments, litigated over trade dress, and sells a competing JUNO-60 plugin. Every emulation on the market (TAL, Cherry Audio, Arturia) keeps the layout and colour coding but uses its own name, lettering, logo and textures.

Recommendation: reproduce the **layout, control types, proportions, colour coding, LED and slider behaviour exactly**, draw the panel in vector with own lettering and an original name and logo, and never use "Roland" or "Juno" in the product name, bundle identifier or plugin codes. The repo name "jnsynth" is fine. Factory patches: ship the 56 as the acceptance-test set in the repo; whether to ship them as the default bank or under community-made names is a licensing judgment for the owner.

### 6. Scope of "full MIDI implementation"

Proposed: note on/off, pitch bend with range, mod wheel to LFO depth, CC64 hold, CC for every panel control with a documented default map, 14-bit CC pairs, NRPN, MIDI learn, program change plus bank select to presets, Juno-106 SysEx parameter and patch messages both in and out (so the plugin can act as an editor for a real 106), MIDI 2.0 UMP accepted where the host delivers it, MPE off by default. Velocity is not part of the original; offer it as an opt-in modern feature.

### 7. Platform scope

macOS only as asked, universal binary, deployment target macOS 11. The code will be portable (JUCE, CMake) so Windows and Linux builds are a later decision, not a rewrite.

## What the owner can do right now to unblock the plan

See `01-research-plan.md` section 5 for the full list. The short version:

1. Confirm decisions 1 to 6 above.
2. Download the two PDFs and work through the checklist in research/02 section 9, or share them privately (do not commit Roland PDFs to the public repo).
3. Say whether a real Juno-60 (and/or 106) is available for a bench session, and what audio and test gear is on hand.
4. Provide high-resolution, straight-on photographs of the panel, bender panel and rear panel with a ruler in frame, for the UI.

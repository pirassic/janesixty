# jnsynth: project overview and decisions

Goal: an open-source, component-modelled (not sampled) recreation of the Roland Juno-60 as a native macOS AU and VST3 plugin and standalone app, with a panel that matches the original in layout and feel plus modern preset management, A/B compare, a full MIDI implementation, and a later Polyend-style scale and chord performance layer.

Companion documents:

- `01-research-plan.md`: what still has to be learned and how, without reference hardware.
- `02-development-plan.md`: architecture, phases, milestones, acceptance criteria.
- `../research/`: compiled research notes. `05-manual-verified-facts.md` is the authority where notes conflict; it was written from a first-hand read of both manuals.

## Decisions taken (2026-10-04)

| # | Decision | Consequence |
|---|---|---|
| 1 | **Juno-60 only.** No 106 mode for now. | One calibration table, one panel, IR3R01 analogue envelopes, 56 patches, DCB-era behaviour. The DSP stays structured so a 106 variant could be added later, but nothing is built for it. |
| 2 | **No reference hardware.** | Fidelity target is defined in section "What identical means without a unit" below. The research plan's bench session is replaced by a document-and-prior-art calibration, with a bench protocol kept on file for anyone who later has a unit. |
| 3 | **AU + VST3 + standalone. AAX dropped.** | No Avid or PACE dependencies, CI can build and sign every artefact, forks can ship everything. |
| 4 | **Original product name**, no Roland or Juno mark in name, bundle id, plugin codes or logo. | Candidates in section "Name proposals". Panel layout, proportions, colour coding and control behaviour are reproduced; lettering, logo and textures are original. |
| 5 | **Velocity and MPE exist as opt-in extras**, off by default. | Implemented as settings toggles outside the panel: velocity routing (to VCA, VCF ENV depth, or both, with amount) and MPE (per-note bend and pressure to the same destinations). When off, the synth behaves exactly like the hardware: no velocity, channel-wide bend only. |
| 6 | **Polyend-style performance layer** (scale mode and chord mode, both toggleable) with Mac keyboard input in the standalone app and pad-controller input with LED feedback, Novation Circuit Rhythm first. | Built as a MIDI input transform ahead of the voice allocator, bypassable, in its own phase after the Juno model is accepted. See development plan phase 7. |

## What "identical" means without a unit

With no hardware to measure, the model cannot be fitted to a specific instrument. What it can be fitted to, in order of authority:

1. **The Service Notes calibration targets.** These define what a correctly adjusted Juno-60 does, in numbers: master clock 1 902 810 Hz, divisor 4305 for 442 Hz; saw 12 Vp-p; PWM 50 % and 95 % endpoints; VCA 4 Vp-p; VCF self-oscillation 248 Hz at FREQ 3 with resonance 4 Vp-p; key follow 1 octave per octave pivoting at C4; VCF LFO full depth sweeping 40 Hz to 5 kHz; VCF ENV full depth reaching 30 kHz; attack 3 s at slider top; LFO 22 Hz at slider top with 14 Vp-p; delay 2 s; noise 4 Vp-p. A unit that passes the adjustment procedure meets these, so a model that meets them matches every well-serviced Juno-60 to within the procedure's tolerances.
2. **The schematic.** Component values and topology for the DCO, mixer, IR3109 cascade, resonance path, HPF, VCA, chorus filters and BBD line. Where the model is a circuit model, the schematic is the specification. Offline SPICE or ACME.jl simulation of these netlists provides frequency responses, harmonic behaviour and step responses that the real-time model is tested against.
3. **Published measurements of real units.** pendragon-andyh's envelope, LFO, chorus and HPF measurements (one Juno-60), Holters and Parker's BBD measurement, and the forum-reported chorus rates. These fill the gaps the schematic leaves: slider laws, chorus delay range, BBD gain.
4. **Commercial emulations as a secondary check.** Roland's own plugin and TAL-U-NO-LX were each calibrated against real units. They are used only to sanity-check slider tapers and depth curves where the hardware documents are silent, never as the target.

What this cannot deliver: the exact tanh drive level into the IR3109 (how much the filter "growls" at full mixer level), the BBD noise floor and clock residue spectrum, the DCO saw flyback shape, per-voice tolerances, and the exact slider taper of every pot. For those the plan uses the schematic-derived value plus a documented uncertainty, exposed as a user-facing "condition" control where it matters audibly (filter drive, chorus noise, voice spread). Fidelity claims in the README will say exactly this.

If a unit becomes available later, the bench protocol in the research plan is ready, and the calibration file is designed to be replaced without touching the DSP code.

## Product name

**Jane-Sixty** (owner's choice, 2026-10-04). Identifiers: `jane60` (bundle id suffix, CMake target, repo paths), display name "Jane-Sixty", vendor "clevergear" (display vendor name changeable later; bundle id and manufacturer code frozen at v1.0). Rationale: no letter sequence or vowel sound shared with the Roland mark; the model number is reused the way TAL (U-NO-LX) and Cherry Audio (DCO-106) do; a quiet nod to the mythology without the word. Rejected: JUNE-Sixty (one letter and one vowel from JUNO, reads as a pun on the mark). Earlier candidates (Hexa-60, DCO-6) are kept as fallbacks if a clearance search finds a conflict. A trademark database search is required before the first tagged release.

## Panel photographs

The owner supplied three photographs, kept outside git in the session scratchpad and to be stored privately by the owner:

1. Straight-on front, 2000 px: layout, slider spacing, colour coding; about 1.9 px/mm against the 1060 mm width.
2. Angled left-end view: slider cap profile (black, chamfered, white index line), the cream LFO TRIG button, the ribbed VOLUME knob with red index, bender lever, end-cheek bevel, rear logo strip.
3. Top-down with power on: lit red LED indicators, red seven-segment display, section header ink colours, VCA LEVEL scale marked -5 to +5.

These cover phase 3 needs. Further public sources in research/06 section 6. Reference photographs are used only to measure layout and proportions and to draw an original vector panel. They are never redistributed in the repo.

## What is needed from the owner now

1. Go-ahead for phase 0.
2. Optional but high value: circulate `research/09-owner-capture-protocol.md` to Juno-60 owners, and download the factory-patch video audio with a timestamp table.

A Launchpad will be borrowed for phase 7. Nothing blocks phase 0.

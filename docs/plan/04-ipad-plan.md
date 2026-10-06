# Jane-Sixty for iPad: plan

Written 2026-10-06 after the v0.9.0 desktop release, for a separate working session. The desktop plugin is the foundation: the instrument in `src/dsp` and the calibration file carry over unchanged. The work is a touch UI, the AUv3 packaging, the App Store pipeline, and a licensing decision that has to be made before the first line of iPad code.

## Goals

- **Jane-Sixty as an AUv3 instrument** that loads in GarageBand, Logic Pro for iPad, Cubasis, AUM, Drambo and any other AUv3 host, with full state save and restore.
- **A standalone iPad app** that hosts the same instrument with an on-screen keyboard, MIDI over USB and Bluetooth, and background audio.
- **Interoperability with Audiobus** and the rest of the iOS audio ecosystem. See the Audiobus section: AUv3 is the mechanism, not the Audiobus SDK.
- Sound bit-identical to the desktop build at the same sample rate. One engine, one calibration file, one set of tests.

## Non-goals for the first iPad release

- iPhone. The panel does not fit; a phone layout is a separate design.
- Inter-App Audio. Apple deprecated it in iOS 13 and hosts have moved to AUv3.
- The performance layer (phase 7 of the development plan). It arrives on iPad when it arrives on desktop.
- A second copy of the engine or the panel. If an iPad need cannot be met by the shared code, the fix goes into the shared code.

## Decisions that gate everything else

1. **JUCE licence.** The desktop builds use JUCE under AGPLv3. App Store terms are incompatible with the GPL family, so the iPad build uses JUCE under its commercial licence. JUCE offers a free tier below a revenue threshold (with a JUCE splash screen) and paid tiers above it. Settle which tier, and sign up at juce.com before the first store build. Nothing in the repository changes: the licence is a per-build choice.
2. **Our own code.** The owner holds the copyright on the whole codebase today, so the store build can carry it under any licence. `CONTRIBUTING.md` now accepts contributions under GPLv3 and MIT with a sign-off, so later contributors do not block store builds. Keep enforcing the sign-off.
3. **Trademarks in the store listing.** Apple removes apps on a trademark complaint without much process. The listing, screenshots, keywords and app name must not use "Roland" or "Juno". Describe the instrument as a component model of "a classic 1982 six-voice polysynth" and keep the trademark disclaimer in the description. The repository and the manual can say Juno-60 as factual reference; the store page is a different risk profile.
4. **Product identity.** Bundle ids: `com.clevergear.jane60.ios` for the app and `com.clevergear.jane60.ios.auv3` for the extension, an app group `group.com.clevergear.jane60` shared by both so presets written in the standalone are visible inside hosts. The manufacturer and plugin codes `Clvg` / `Jn60` stay; the AUv3 is the same instrument to a host that also knows the desktop AU.

## Phase A: build the engine on iPad (one to two days)

- CMake with the Xcode generator and the iOS system name: `cmake -S . -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0`. JUCE requires Xcode for iOS; Ninja will not do here.
- `juce_add_plugin` gains `FORMATS AUv3 Standalone` for the iOS configuration, plus `BACKGROUND_AUDIO_ENABLED TRUE`, `BLUETOOTH_PERMISSION_ENABLED TRUE` (Bluetooth MIDI pairing), `APP_GROUPS_ENABLED TRUE` with `APP_GROUP_IDS group.com.clevergear.jane60`, `IPAD_SCREEN_ORIENTATIONS UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight`, `REQUIRES_FULL_SCREEN TRUE`, `STATUS_BAR_HIDDEN TRUE`. Keep this in an `if(IOS)` block so the desktop targets are untouched.
- The icon: JUCE builds the iOS asset catalog from the same `ICON_BIG` and `ICON_SMALL`. Add a 1024 px App Store icon without transparency (App Store rejects alpha in the marketing icon); the current master has transparent corners, so export a flattened variant on the panel colour.
- Run the standalone on a device through Xcode. Measure CPU with six voices, chorus on, at 48 kHz and 256 samples: the budget is comfortable, but AUv3 extensions have a memory ceiling that is lower than an app's, so note the resident size as well.
- Unit tests: the Catch2 suite is Linux-only in CI and that is fine; the engine does not change. If an iOS-only numeric difference ever appears, it is a compiler flag question (`-ffast-math` is off everywhere; keep it off).
- Exit: the standalone makes sound on an iPad from a USB keyboard; CPU and memory numbers are written into the handover.

## Phase B: touch UI (the main work, one to two weeks)

The editor (`src/plugin/PluginEditor.*`, `src/ui/*`) is desktop-sized, mouse-driven, and reaches its settings through a title-bar button that does not exist on iOS.

- **Scaling.** Make the editor resizable with a fixed aspect ratio and a scale factor applied to the whole panel, so an AUv3 host can give it any width and the panel fits. JUCE's `setResizable` with `setFixedAspectRatio` plus a transform on the content is the usual route. Logic for iPad and GarageBand constrain the view height; AUM gives the full screen.
- **Hit targets.** Apple's minimum is 44 pt. The slider caps and the small buttons (LFO trigger, chorus I/II, range) are under that at panel scale on an 11-inch iPad. Enlarge the touch area without changing the look: hit testing on a larger rectangle than the painted cap.
- **Gestures.** Sliders already drag. Add a fine-adjust mode (two-finger drag or a hold-then-drag), double-tap to reset, and make sure multi-touch moves several sliders at once, which JUCE supports but the panel's mouse-capture logic must not block.
- **Settings.** The desktop Standalone puts Settings in the window title bar; the plugin editor has an in-panel settings button that is hidden in Standalone. On iOS always show the in-panel button, and present the settings as a touch-friendly list rather than a `PopupMenu` (popup menus work on iOS but are small).
- **Keyboard.** The standalone needs the on-screen keyboard component always visible, octave buttons, and the hold and arpeggiator controls reachable with a thumb. The computer-keyboard shortcuts and the "Keyboard shortcuts" dialog are desktop-only; hide them with a platform check.
- **Preset browser.** The current preset bar is a desktop list. On iPad it should be a sheet with the 56 factory patches in their bank layout and the user presets below. Save-as needs the on-screen text keyboard; JUCE handles it, test that it does not cover the panel.
- **Demo player.** Keep; it is a good first-run experience. Make sure its transport is touchable.
- Exit: every control usable with a finger on an 11-inch and a 13-inch iPad, panel readable in Logic's constrained view, nothing requires a keyboard or a pointer.

## Phase C: AUv3 behaviour (two to four days)

- **State.** AUv3 hosts save the full state through `getStateInformation`; the condition and extras settings are stored in that state, so they travel with the project. Verify save, reload, duplicate track.
- **Presets.** The AUv3 extension runs in its own sandbox; the app group container is where user presets live on iOS. `PresetManager::userFolder()` gets an iOS branch that returns the app group container path. Factory presets are embedded, no change.
- **Parameters.** Hosts expose the parameter list for automation; the names and ranges come from `Parameters.cpp` and are already host-friendly on desktop. Check that none exceeds what GarageBand displays.
- **MIDI.** In AUv3 the host delivers MIDI; MPE works if the host sends per-note channels (AUM does, GarageBand does not). The MPE opt-in stays in the settings.
- **Sample rate and buffer.** Hosts run at 44.1 or 48 kHz with buffers down to 64 samples. The oversampled filter and the chorus are stateless across buffer sizes; just test the small buffers for CPU.
- **View lifecycle.** Hosts open and close the view often. The editor must create and destroy cheaply, and the demo player must stop when the view goes away.
- Exit: Logic Pro for iPad, GarageBand, AUM and Cubasis each load the instrument, save and restore a project, and show the panel correctly.

## Phase D: interoperability (one to two days)

- **Audiobus.** Audiobus 3 hosts AUv3 plug-ins directly: once the AUv3 is installed, Jane-Sixty appears in Audiobus as an instrument and can be routed into any other Audiobus-aware app. That is the interoperability requirement met, with no SDK. The Audiobus SDK proper is a closed-source library for apps that want to appear as "Audiobus apps" through their standalone; it needs registration with Audiobus, an app review by them, and a binary dependency that cannot ship in a GPL build. Recommendation: rely on AUv3 for the first release, revisit the SDK only if users ask for standalone-level Audiobus features. If it is added later, it goes into the store build only, behind a build flag, and never into the public GPL tree.
- **Ableton Link** for the arpeggiator's tempo: the LinkKit SDK is a binary too, same treatment. Nice to have, not for the first release. The arpeggiator follows host tempo in AUv3 anyway through the host's transport.
- **Bluetooth MIDI.** JUCE's pairing dialogue works; expose it from the settings list in the standalone.
- **Background audio** in the standalone so a Bluetooth keyboard keeps playing while another app is in front.
- Exit: Jane-Sixty plays into Audiobus and AUM from the AUv3; the standalone plays in the background from a Bluetooth keyboard.

## Phase E: store pipeline (two to three days)

- **Certificates.** A new "Apple Distribution" certificate (the Developer ID certificates do not cover iOS) and an App Store provisioning profile for each of the two bundle ids, with the app group capability enabled on both App IDs in the developer portal.
- **CI.** A new job in `release.yml`, `ios`, that runs on macOS: `xcodebuild archive` of the standalone target (the extension is embedded), `xcodebuild -exportArchive` with an App Store export options plist, then upload to App Store Connect. The App Store Connect API key already in the secrets (the notary key) works for the upload with `xcrun altool --upload-app --type ios --apiKey --apiIssuer`, so no new secret beyond the distribution certificate and the profiles.
- **Listing.** App Store Connect record, screenshots for 11-inch and 13-inch iPads, privacy answers (no data collection), age rating, support URL (the repository), the trademark-safe description from the decisions section. Price: free, matching the desktop.
- **TestFlight** first: internal testers, then a public link in the README for a beta round before the store submission.
- **Review.** Apple's reviewers try the app with no MIDI gear attached, so the on-screen keyboard and the demo player must make sound on first launch with no setup.
- Exit: a TestFlight build installs from the link; the store submission is accepted.

## Phase F: release and after

- Version numbering follows the desktop: the first iPad release is whatever the desktop is at that moment. The `project(VERSION)` is shared.
- The handover and the user manual gain an iPad section: installing from the store, where presets live, host notes.
- Keep the desktop and iPad builds in one release workflow so a tag produces everything.

## Risks

| Risk | Handling |
|---|---|
| JUCE commercial licence cost at the paid tiers | Settle the tier before phase A; the free tier with the splash screen is acceptable for a free app |
| Trademark complaint on the store listing | No Roland or Juno in any store-visible field; disclaimer in the description |
| AUv3 memory ceiling with the chorus's lookup tables | Measure in phase A; the tables are small, but confirm |
| Host view constraints make the panel unreadable | Scalable panel with a minimum size, verified in Logic and GarageBand in phase C |
| Audiobus SDK wanted after all | Store-build-only flag, never in the GPL tree; AUv3 covers the common case |
| Apple review rejects for no sound on first launch | Demo player and on-screen keyboard available with no setup |

## Order of work for the next session

1. Decide the JUCE tier and create the iOS App IDs with the app group (owner, outside the code).
2. Phase A on a device, write the numbers down.
3. Phase B in two passes: scaling and hit targets first, then the preset sheet and settings list.
4. Phase C against the four hosts.
5. Phase D, E, F in order.

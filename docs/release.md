# Releasing Jane-Sixty

## Once: Apple signing and notarisation (owner)

Needs an active Apple Developer Program membership.

1. **Certificates.** In Xcode, Settings, Accounts, select the team, Manage Certificates, and create two: **Developer ID Application** and **Developer ID Installer**. (Or on developer.apple.com, Certificates, with a signing request made in Keychain Access.) Both land in the login keychain.
2. **Export them as one .p12.** In Keychain Access, My Certificates, select both Developer ID certificates (each with its private key), right-click, Export 2 items, format .p12, choose a password. Then in a terminal:
   `base64 -i developer-id.p12 | pbcopy`
   and paste the result into the repository secret `MACOS_CERTIFICATE_P12`; put the password in `MACOS_CERTIFICATE_PASSWORD`.
3. **Team id.** developer.apple.com, Membership details, the 10-character Team ID, into `APPLE_TEAM_ID`.
4. **Notarisation key.** App Store Connect, Users and Access, Integrations, App Store Connect API, Team keys: generate a key with the **Developer** role. Note the Key ID and the Issuer ID, download the .p8 (one chance). Secrets: `NOTARY_KEY_ID`, `NOTARY_ISSUER_ID`, and `NOTARY_KEY_P8` as `base64 -i AuthKey_XXXX.p8 | pbcopy`.

Secrets go under the repository's Settings, Secrets and variables, Actions. The release workflow reads nothing else. Without the certificate secret it still builds and packages, unsigned, with a warning in the log; without the notary secrets it signs but does not notarise.

Status 2026-10-06: all six secrets are set and the signed path is verified (Release run 2: signed, notarised, stapled, Gatekeeper "Notarized Developer ID").

## Each release

1. Bump `project(Jane60 VERSION x.y.z)` in `CMakeLists.txt` (the plugin reports this version to hosts) and merge to the branch being released.
2. Tag and push: `git tag v0.9.0 && git push origin v0.9.0`. The first public release is 0.9; 1.0 follows the owner's listening pass and host checks.
   If the draft turns out wrong before it is published, delete the draft release on GitHub, delete the tag (`git push origin :refs/tags/v0.9.0`), fix, and tag the new commit with the same name. A tag only moves while its release is still a draft.
3. The **Release** workflow builds macOS (universal, signed, notarised, stapled `.pkg` plus bare-bundle zips) and Windows (Inno Setup installer plus zips), then opens a **draft** GitHub release with the assets and generated notes. About 25 minutes.
4. Check the draft: install the `.pkg` on a clean Mac (no quarantine prompt means notarisation worked), run the Windows installer in a VM, then publish.

A dry run without publishing: push the commit to a branch named `release-test/<anything>`; the build and installer jobs run, the release job does not.

## Windows signing (optional)

The installer and the VST3 are unsigned; SmartScreen warns until the files gain reputation. A code-signing certificate (OV or EV) would be added as a `signtool` step before the Inno Setup step. Not planned before 1.0.

## What the installers do

- macOS `.pkg`: three selectable components, the AU to `/Library/Audio/Plug-Ins/Components`, the VST3 to `/Library/Audio/Plug-Ins/VST3`, the app to `/Applications`. Sources in `installer/macos/`.
- Windows setup: the VST3 to `Common Files\VST3`, the app to `Program Files\clevergear\Jane-Sixty` with a Start Menu entry and an uninstaller. Source in `installer/windows/jane60.iss`.

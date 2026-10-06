# Security policy

Jane-Sixty is an audio plug-in and standalone application. It opens no network connections, reads only its own preset and settings files, and runs no code from the internet. The attack surface is the files it parses: presets (`.json`), plug-in state saved by a host, and MIDI.

## Reporting a vulnerability

Please do not open a public issue for a security problem. Use GitHub's private vulnerability reporting on this repository (Security tab, "Report a vulnerability"), which reaches the maintainer only. Include the version, the platform, and a file or a sequence of steps that reproduces the problem. You will get an acknowledgement within a week.

## Supported versions

Only the latest release receives fixes.

## Release integrity

macOS installers are signed with a Developer ID certificate and notarised by Apple; Gatekeeper reports "Notarized Developer ID". Windows installers are not yet code-signed, so SmartScreen may warn. Download installers only from this repository's Releases page.

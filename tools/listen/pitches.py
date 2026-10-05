#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Estimates the notes played in each reference segment (dominant fundamental per 100 ms frame,
by harmonic product spectrum) and writes a phrase file for RenderPatches: one line per note,
'patch startSeconds midiNote durationSeconds'. The render then sits in the demo's register.

The demo player accompanies most patches with a left-hand bass line (82 to 150 Hz on the
harpsichords, guitar, celesta) that the dominant-pitch estimate misses behind the melody.
Its register is read separately from the 40 to 150 Hz band in frames where that band sits
within BASS_DB of the broadband level, and a bass note is added under the triad and alone.
Without it the plugin renders measured 6 dB light below 150 Hz against the recording
(listening notes, 2026-10-05)."""
import csv, sys
import numpy as np
sys.path.insert(0, "tools/listen")
import compare_reference as c

SR = 48000
BASS_DB = 18   # sub band within this of the 150 Hz to 6 kHz band counts as a played bass note

def bandpass(x, lo, hi):
    X = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / SR)
    X[(f < lo) | (f > hi)] = 0
    return np.fft.irfft(X, len(x))

def bass_note(seg):
    """Median MIDI note of the sub band where it carries a played note, else None."""
    sub = bandpass(seg, 40, 150)
    bb = bandpass(seg, 150, 6000)
    fr = int(0.2 * SR)
    n = len(seg) // fr
    notes = []
    for i in range(n):
        a, b = i * fr, (i + 1) * fr
        ls = np.sqrt((sub[a:b] ** 2).mean()) + 1e-9
        lb = np.sqrt((bb[a:b] ** 2).mean()) + 1e-9
        if 20 * np.log10(ls / lb) < -BASS_DB:
            continue
        sp = np.abs(np.fft.rfft(sub[a:b] * np.hanning(fr), 4 * fr))
        f = np.fft.rfftfreq(4 * fr, 1 / SR)
        sel = (f >= 40) & (f < 150)
        notes.append(int(round(69 + 12 * np.log2(f[sel][sp[sel].argmax()] / 440.0))))
    if len(notes) < max(3, n // 5):   # a bass line, not an onset thump: a fifth of the frames
        return None
    return int(np.median(notes))

def frame_pitch(x):
    n = len(x)
    w = np.hanning(n)
    sp = np.abs(np.fft.rfft(x * w, 4 * n))
    f = np.fft.rfftfreq(4 * n, 1 / SR)
    hps = sp.copy()
    for h in (2, 3, 4):
        d = sp[::h]
        hps[:len(d)] *= d
    sel = (f >= 55) & (f <= 2000)
    i = np.argmax(hps[sel])
    return float(f[sel][i])

def main():
    x = np.fromfile("reference/ref48k.f32", dtype=np.float32).reshape(-1, 2).astype(float).mean(axis=1)
    segs = c.segments("reference/labels.txt")
    with open("calibration/factory_patches.csv") as f:
        patches = [(int(r[0]), r[1]) for r in csv.reader(f) if r and r[0].isdigit()]
    out = []
    for num, name in patches:
        s, e = segs[name]
        seg = x[int((s + 0.5) * SR):int((e - 0.5) * SR)]
        fr = int(0.1 * SR)
        n = len(seg) // fr
        frames = seg[:n * fr].reshape(n, fr)
        rms = np.sqrt((frames ** 2).mean(axis=1))
        loud = rms > rms.max() * 0.2
        notes = [int(round(69 + 12 * np.log2(frame_pitch(fr_) / 440.0))) for fr_, l in zip(frames, loud) if l]
        notes = [m for m in notes if 36 <= m <= 96]
        if not notes:
            notes = [60]
        q = np.percentile(notes, [25, 50, 75]).astype(int)
        # Phrase: three single notes at the demo's quartile pitches, 1.5 s each with 1 s gaps,
        # then a triad at the median for 2 s.
        t = 0.0
        for m in q:
            out.append(f"{num} {t:.2f} {m} 1.5")
            t += 2.5
        for m in (q[1], q[1] + 4, q[1] + 7):
            out.append(f"{num} {t:.2f} {m} 2.0")
        bass = bass_note(seg)
        if bass is not None and bass < q[1]:
            out.append(f"{num} {t:.2f} {bass} 2.0")      # under the triad
            out.append(f"{num} {t + 2.5:.2f} {bass} 1.5")  # and alone
        print(num, name, "notes", [int(v) for v in q], "bass", bass)
    with open("reference/phrases.txt", "w") as f:
        f.write("\n".join(out) + "\n")

if __name__ == "__main__":
    main()

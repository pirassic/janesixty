#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Estimates the notes played in each reference segment (dominant fundamental per 100 ms frame,
by harmonic product spectrum) and writes a phrase file for RenderPatches: one line per note,
'patch startSeconds midiNote durationSeconds'. The render then sits in the demo's register."""
import csv, sys
import numpy as np
sys.path.insert(0, "tools/listen")
import compare_reference as c

SR = 48000

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
        print(num, name, "notes", list(q))
    with open("reference/phrases.txt", "w") as f:
        f.write("\n".join(out) + "\n")

if __name__ == "__main__":
    main()

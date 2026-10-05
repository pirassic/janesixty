#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Per-patch comparison of the plugin renders (tools/listen/RenderPatches.cpp) against the
factory demo recording, on measures that do not depend on the notes played: band balance,
spectral tilt, stereo width and attack time. Inputs live under reference/ (gitignored)."""
import argparse, csv, math, os
import numpy as np

SR = 48000
BANDS = [("sub", 40, 150), ("low", 150, 500), ("mid", 500, 2000), ("himid", 2000, 6000), ("high", 6000, 15000)]

def welch(x, n=8192):
    w = np.hanning(n)
    hop = n // 2
    acc = np.zeros(n // 2 + 1)
    cnt = 0
    for i in range(0, len(x) - n, hop):
        acc += np.abs(np.fft.rfft(x[i:i + n] * w)) ** 2
        cnt += 1
    return np.fft.rfftfreq(n, 1 / SR), acc / max(cnt, 1)

def measures(st):
    """st: (N, 2) float. Returns dict of measures."""
    mid = st.mean(axis=1)
    side = 0.5 * (st[:, 0] - st[:, 1])
    f, p = welch(mid)
    total = p[(f >= 40) & (f <= 15000)].sum()
    m = {}
    for name, lo, hi in BANDS:
        m[name] = 10 * math.log10(p[(f >= lo) & (f < hi)].sum() / total + 1e-15)
    sel = (f >= 100) & (f <= 10000)
    slope, _ = np.polyfit(np.log2(f[sel]), 10 * np.log10(p[sel] + 1e-20), 1)
    m["tilt"] = slope  # dB per octave
    m["width"] = max(-40.0, 10 * math.log10((side ** 2).mean() / ((mid ** 2).mean() + 1e-15) + 1e-15))
    # attack: envelope in 2 ms frames; onsets where the envelope rises 12 dB within 50 ms; time from -20 dB to -1 dB re the local peak
    fr = int(0.002 * SR)
    n = len(mid) // fr
    env = np.sqrt((mid[:n * fr].reshape(n, fr) ** 2).mean(axis=1))
    db = 20 * np.log10(env + 1e-9)
    att = []
    i = 25
    while i < n - 150:
        if db[i] - db[i - 25] > 12 and db[i] > db.max() - 30:
            pk = i + np.argmax(db[i:i + 150])
            peak = db[pk]
            j = pk
            while j > 0 and db[j] > peak - 20: j -= 1
            k = j
            while k < pk and db[k] < peak - 1: k += 1
            att.append((k - j) * 0.002)
            i = pk + 150
        else:
            i += 1
    m["attack_ms"] = 1000 * float(np.median(att)) if att else float("nan")
    m["onsets"] = len(att)
    return m

def segments(path):
    """Chapter labels, with the four hidden patches split by silence (checked by hand)."""
    segs = {}
    with open(path) as f:
        for line in f:
            s, e, name = line.rstrip("\n").split("\t")
            segs[name] = (float(s), float(e))
    segs["Organ 1"] = (55.0, 63.7); segs["Organ 2"] = (65.15, 73.55); segs["Organ 3"] = (74.95, 83.9); segs["Brass"] = (84.6, 92.8)
    segs["Trumpet"] = (327.0, 333.8); segs["Horn"] = (336.75, 344.05); segs["Tuba"] = (348.0, 356.8)
    segs["Violine"] = segs.pop("Violine (sic)")
    return segs

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ref", default="reference/ref48k.f32")
    ap.add_argument("--labels", default="reference/labels.txt")
    ap.add_argument("--render", default="reference/render")
    ap.add_argument("--out", default="reference/comparison.md")
    a = ap.parse_args()
    x = np.fromfile(a.ref, dtype=np.float32).reshape(-1, 2).astype(float)

    segs = segments(a.labels)

    with open("calibration/factory_patches.csv") as f:
        patches = [(int(r[0]), r[1]) for r in csv.reader(f) if r and r[0].isdigit()]

    rows = []
    for num, name in patches:
        if name not in segs:
            print("no segment for", num, name); continue
        s, e = segs[name]
        ref = x[int((s + 0.5) * SR):int((e - 0.5) * SR)]
        rp = os.path.join(a.render, f"patch_{num:02d}.f32")
        ren = np.fromfile(rp, dtype=np.float32).reshape(-1, 2).astype(float)
        mr, mp = measures(ref), measures(ren)
        rows.append((num, name, mr, mp))

    keys = [b[0] for b in BANDS] + ["tilt", "width", "attack_ms"]
    lines = ["# Factory patches: plugin render vs factory demo recording", "",
             "Band values are dB of the band relative to the 40 Hz to 15 kHz total (mid channel); diff = plugin minus reference. tilt in dB/octave over 100 Hz to 10 kHz; width = side/mid energy in dB; attack = median 20 dB rise time at onsets. The reference phrases differ from the render phrase, so single-patch band differences under about 3 dB are within the method's noise; look at the direction across many patches.", "",
             "| # | patch | " + " | ".join(f"{k} ref / plug / diff" for k in keys) + " |", "|---|---|" + "---|" * len(keys)]
    score = []
    for num, name, mr, mp in rows:
        cells = []
        for k in keys:
            d = mp[k] - mr[k]
            cells.append(f"{mr[k]:.1f} / {mp[k]:.1f} / {d:+.1f}")
        lines.append(f"| {num} | {name} | " + " | ".join(cells) + " |")
        score.append((sum(abs(mp[k] - mr[k]) for k in ["sub", "low", "mid", "himid", "high"]), num, name))
    mean = {k: float(np.mean([mp[k] - mr[k] for _, _, mr, mp in rows if not math.isnan(mp[k] - mr[k])])) for k in keys}
    lines += ["", "## Mean difference over all patches (plugin minus reference)", "", "| " + " | ".join(keys) + " |", "|" + "---|" * len(keys), "| " + " | ".join(f"{mean[k]:+.2f}" for k in keys) + " |"]
    lines += ["", "## Patches furthest from the reference (sum of absolute band differences)", ""]
    for sc, num, name in sorted(score, reverse=True)[:12]:
        lines.append(f"- {num} {name}: {sc:.1f} dB")
    with open(a.out, "w") as f:
        f.write("\n".join(lines) + "\n")
    print("\n".join(lines[-20:]))
    print("mean:", {k: round(v, 2) for k, v in mean.items()})

if __name__ == "__main__":
    main()

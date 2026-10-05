#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compares the plugin block dump with the ngspice reference and writes a Markdown report.
Thresholds are reported, not enforced (research plan: tracked in CI as a report)."""
import argparse, csv, os, math

def load(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ref", required=True)
    ap.add_argument("--dsp", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    lines = ["# IR3109: plugin block vs ngspice reference", ""]
    ok = True

    r = {float(x["freqHz"]): float(x["gainDb"]) for x in load(os.path.join(a.ref, "response.csv"))}
    d = {float(x["freqHz"]): float(x["gainDb"]) for x in load(os.path.join(a.dsp, "response.csv"))}
    lines += ["## Small-signal response, FC 1 kHz, k 0 (tolerance 1 dB)", "", "| f (Hz) | reference dB | plugin dB | diff |", "|---|---|---|---|"]
    for k in r:
        diff = d[k] - r[k]
        flag = "" if abs(diff) <= 1.0 else " (!)"
        ok &= abs(diff) <= 1.0
        lines.append(f"| {k:g} | {r[k]:.2f} | {d[k]:.2f} | {diff:+.2f}{flag} |")

    r = load(os.path.join(a.ref, "selfosc.csv"))[0]
    d = load(os.path.join(a.dsp, "selfosc.csv"))[0]
    fr, fd = float(r["oscHz"]), float(d["oscHz"])
    vr, vd = float(r["vpp"]), float(d["vpp"])
    lines += ["", "## Self-oscillation at the plugin's corner and kMax (tolerance 2 % frequency, 10 % level)", "",
              "| | reference | plugin |", "|---|---|---|",
              f"| corner (Hz) | {float(r['cornerHz']):.1f} | {float(d['cornerHz']):.1f} |",
              f"| oscillation (Hz) | {fr:.2f} | {fd:.2f} |",
              f"| Vp-p | {vr:.3f} | {vd:.3f} |",
              f"| osc / corner | {fr / float(r['cornerHz']):.4f} | {fd / float(d['cornerHz']):.4f} |"]
    ok &= abs(fd - fr) / fr <= 0.02 and abs(vd - vr) / vr <= 0.10

    r = {int(x["harmonic"]): float(x["dbReH1"]) for x in load(os.path.join(a.ref, "harmonics.csv"))}
    d = {int(x["harmonic"]): float(x["dbReH1"]) for x in load(os.path.join(a.dsp, "harmonics.csv"))}
    lines += ["", "## Harmonics of a 12 Vp-p 220 Hz sine, FC 2 kHz, k 0 (tolerance 2 dB)", "", "| H | reference dB | plugin dB | diff |", "|---|---|---|---|"]
    for k in r:
        rv, dv = max(r[k], -90.0), max(d[k], -90.0)   # below -90 dB both are "absent"
        diff = dv - rv
        flag = "" if abs(diff) <= 2.0 else " (!)"
        ok &= abs(diff) <= 2.0
        lines.append(f"| {k} | {rv:.1f} | {dv:.1f} | {diff:+.1f}{flag} |")

    r = load(os.path.join(a.ref, "passband.csv"))
    d = {float(x["k_or_gm"]): float(x["gain100HzDb"]) for x in load(os.path.join(a.dsp, "passband.csv"))}
    lines += ["", "## Passband gain at 100 Hz vs resonance", "", "| mode | k or gm | reference dB | plugin dB |", "|---|---|---|---|"]
    for x in r:
        pd = d.get(float(x["k_or_gm"])) if x["mode"] == "model" else None
        lines.append(f"| {x['mode']} | {x['k_or_gm']} | {float(x['gain100HzDb']):.2f} | {'' if pd is None else f'{pd:.2f}'} |")
    lines += ["", "The `network` rows run the p.9 resistor network as drawn with the BA662 as a plain transconductance, for both input polarities; they answer whether the drawn network reproduces the ~7 dB passband loss at full resonance that the `model` compensation (coefficient 0.308) gives.", ""]
    lines.append("**Result: all within tolerance.**" if ok else "**Result: at least one measurement outside tolerance (!).**")
    with open(a.out, "w") as f:
        f.write("\n".join(lines) + "\n")
    print("\n".join(lines))

if __name__ == "__main__":
    main()

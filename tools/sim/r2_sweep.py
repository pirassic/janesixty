# SPDX-License-Identifier: GPL-3.0-or-later
# Hypothesis test: the BA662 inputs are bare bases (owner's internal schematic, 2026-10-06), so the
# compensation leg's weakness must be in the external network. Sweep R2 (the shunt on pin 3) and
# report the passband at 100 Hz vs normalised resonance, next to the plugin model's 0.308 rows.
import sys, os, tempfile, numpy as np
sys.path.insert(0, "tools/listen"); sys.path.insert(0, "tools/sim")
import ir3109_ref as R
orig = R.netlist
def run_variant(r2):
    def nl(mode, fc, **kw):
        return orig(mode, fc, **kw).replace("R2 np 0 1.5k", f"R2 np 0 {r2}")
    R.netlist = nl
    wd = tempfile.mkdtemp()
    def grows(gm):
        t, y = R.tran(wd, "network", 1000.0, tstop=0.06, tstep=1.0 / 192000, gm=gm, amp=0.0, freq=100.0)
        a = np.abs(y - y.mean()); q = len(a) // 4
        return a[3 * q:].max() > 1e-3
    lo, hi = 0.0, 0.1
    for _ in range(14):
        mid = 0.5 * (lo + hi)
        if grows(mid): hi = mid
        else: lo = mid
    gm_osc = 0.5 * (lo + hi)
    out = []
    for kn in [0.0, 1.0, 2.0, 3.0, 3.9]:
        out.append(R.ac_gain_db(wd, "network", 1000.0, [100.0], gm=gm_osc * kn / 4.0)[0])
    R.netlist = orig
    return gm_osc, out
wd = tempfile.mkdtemp()
model = [R.ac_gain_db(wd, "model", 1000.0, [100.0], k=k)[0] for k in [0.0, 1.0, 2.0, 3.0, 3.9]]
print("passband gain at 100 Hz (dB) vs resonance k = 0 / 1 / 2 / 3 / 3.9, relative to k = 0")
print(f"{'plugin model (0.308)':24} " + " ".join(f"{v - model[0]:+6.2f}" for v in model))
for r2 in ["1.5k", "680", "330", "220", "150", "100"]:
    gm, g = run_variant(r2)
    print(f"{'network R2 = ' + r2:24} " + " ".join(f"{v - g[0]:+6.2f}" for v in g) + f"   (gm_osc {gm * 1e3:.1f} mA/V)")

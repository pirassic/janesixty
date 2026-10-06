#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""ngspice reference for the IR3109 four-pole with BA662 resonance feedback.

Generates netlists, runs ngspice in batch mode and writes the measurements the
plugin block is compared against (tools/sim/compare.py). The OTA cells are
behavioural (no transistor-level IR3109 model exists): each stage is
C dV/dt = I0 tanh(A (Vin - Vout) / 2Vt) with A the 68 k / 560 input divider,
which is the same physics the plugin discretises. The value of the reference is
the continuous-time solution (ngspice's variable-step integrator) against the
plugin's TPT / Newton / 2x path, and the resonance network drawn on Service
Notes p.9 (R14 10 k, R5 47 k, R3 100 k, R1 1.5 k) run as drawn.

Modes:
  model    loop sum as the plugin forms it: x1 = in (1 + comp k) - k y4
  network  the p.9 resistor network, stage 1 wired as drawn (summing node), with the BA662 as a differential
           transconductance: compensation leg R5 47 k into pin 3 (+) with R2
           1.5 k, feedback leg R3 100 k into pin 2 (-) with R1 1.5 k, output
           current into node A (R14 10 k from the mixer, R7 68 k / R6 560 load).
           Its resonance axis is normalised by the simulated oscillation
           threshold: k = 4 * GM / GM_osc.

Outputs (CSV in --out):
  response.csv   mode=model, k=0, FC=1 kHz: gain in dB at 100, 1k, 4k, 8k Hz
  selfosc.csv    mode=model, FC=248*shift, k=4.05: oscillation frequency and Vp-p
  harmonics.csv  mode=model, k=0, FC=2 kHz, 12 Vp-p 220 Hz sine: H2..H5 in dB re H1
  passband.csv   gain at 100 Hz vs k (model) and vs normalised k (network)
  network.csv    the network's oscillation threshold GM_osc and its self-oscillation at k 4.05
"""
import argparse, csv, math, os, subprocess, sys, tempfile
import numpy as np

VT2 = 0.052
STAGE1_DRIVE_DB = -8.8   # calibration vcf.stage1DriveDb: mixer divider and summing-node gain, 2026-10-06
A = 560.0 / 68560.0 * 10 ** (STAGE1_DRIVE_DB / 20.0)
C = 240e-12

def stages(src):
    s = []
    prev = src
    for n in range(1, 5):
        s.append(f"B{n} 0 y{n} I = I0*tanh(ATT*(v({prev})-v(y{n}))/VT2)")
        s.append(f"C{n} y{n} 0 {{CSTG}}")
        prev = f"y{n}"
    return "\n".join(s)

def netlist(mode, fc, k=0.0, comp=0.308, gm=0.0, amp=0.1, freq=100.0, analysis=""):
    i0 = 2 * math.pi * fc * C * VT2 / A
    head = f"""* Jane-Sixty IR3109 reference ({mode})
.param VT2={VT2} ATT={A} CSTG={C} I0={i0}
.option reltol=1e-5 abstol=1e-12 vntol=1e-9 method=gear
Vin mix 0 dc 0 ac 1 sin(0 {amp} {freq})
* small kick so a self-oscillation can start in a noiseless simulation
Vkick kick 0 pulse(0 0.05 0 1u 1u 200u 10)
"""
    if mode == "model":
        body = f"Bsum x1 0 V = (v(mix)+v(kick))*(1+{comp}*{k}) - {k}*v(y4)\n" + stages("x1")
    else:
        body = f"""* Service Notes p.9 (IR3109 pins, read 2026-10-06): R14 10 k from the mixer, the BA662 output and
* R6 560 all meet at the pin 2 node (stage 1 input); R7 68 k returns stage 1's own output (pin 4) to
* that node, as R8 / R11 / R16 do for the other stages. The input node is therefore a current-summing
* node with gain 68 k / 10 k = 6.8 for the mixer and 68 k for the resonance current, which is what
* makes the compensation coefficient (47 k / 1.5 k against 100 k / 1.5 k) / 6.8 = 0.308.
R14 mix na 10k
R6 na 0 560
Einv y1n 0 y1 0 -1
R7 y1n na 68k
Rkick kick na 10Meg
* BA662 (+) input: R5 47 k from the mixer side of R14, R2 1.5 k to ground (compensation)
R5 mix np 47k
R2 np 0 1.5k
* IR3109 output buffer (P-MOS followers on the chip): the feedback leg must not load the stage capacitor
Ebuf y4b 0 y4 0 1
* BA662 (-) input: R3 100 k from the buffered filter output, R1 1.5 k to ground (feedback)
R3 y4b nm 100k
R1 nm 0 1.5k
* BA662 output current into node A; small-signal gm = GM, tanh limited at 2Vt
Bres 0 na I = {gm}*VT2*tanh((v(np)-v(nm))/VT2)
""" + stages("na").replace("B1 0 y1 I = I0*tanh(ATT*(v(na)-v(y1))/VT2)", "B1 0 y1 I = I0*tanh(v(na)/VT2)")
    return head + body + "\n" + analysis + "\n.end\n"

def run(net, workdir, outname):
    path = os.path.join(workdir, outname + ".cir")
    with open(path, "w") as f:
        f.write(net)
    r = subprocess.run(["ngspice", "-b", path], cwd=workdir, capture_output=True, text=True, timeout=600)
    if r.returncode != 0:
        sys.stderr.write(r.stdout[-2000:] + r.stderr[-2000:])
        raise RuntimeError(f"ngspice failed on {outname}")
    return r.stdout

def read_wrdata(path):
    # wr_vecnames puts a header line first; wr_singlescale writes the scale once. The scale of
    # an AC run can be written as two columns (complex), so take the first and last columns.
    d = np.loadtxt(path, skiprows=1)
    return d[:, 0], d[:, -1]

def ac_gain_db(workdir, mode, fc, freqs, **kw):
    ctl = f""".control
set wr_vecnames
set wr_singlescale
ac dec 200 10 100k
wrdata {workdir}/ac.txt vdb(y4)
quit
.endc"""
    run(netlist(mode, fc, analysis=ctl, **kw), workdir, "ac")
    f, g = read_wrdata(os.path.join(workdir, "ac.txt"))
    return [float(np.interp(x, f, g)) for x in freqs]

def tran(workdir, mode, fc, tstop, tstep, **kw):
    ctl = f""".control
set wr_vecnames
set wr_singlescale
tran {tstep} {tstop} 0 {tstep}
wrdata {workdir}/tr.txt v(y4)
quit
.endc"""
    run(netlist(mode, fc, analysis=ctl, **kw), workdir, "tr")
    return read_wrdata(os.path.join(workdir, "tr.txt"))

def dominant(t, y, fmin, fmax):
    dt = t[1] - t[0]
    w = np.hanning(len(y))
    sp = np.abs(np.fft.rfft((y - y.mean()) * w))
    f = np.fft.rfftfreq(len(y), dt)
    m = (f >= fmin) & (f <= fmax)
    i = np.argmax(sp[m])
    return float(f[m][i])

def harmonic_db(t, y, f0, nmax):
    dt = t[1] - t[0]
    n = len(y)
    out = []
    for h in range(1, nmax + 1):
        ph = 2 * np.pi * h * f0 * t
        re = np.sum(y * np.cos(ph)) * 2 / n
        im = np.sum(y * np.sin(ph)) * 2 / n
        out.append(math.hypot(re, im))
    return [20 * math.log10(a / out[0] + 1e-15) for a in out[1:]]

def diag(wd):
    """Operating point and small-signal checks of the network netlist at gm = 0, printed to stdout."""
    ctl = f""".control
set wr_vecnames
set wr_singlescale
dc Vin 1 1 1
print v(mix) v(na) v(s1) v(np) v(nm) v(y1) v(y4)
ac dec 10 10 10k
print vdb(y4)
quit
.endc"""
    net = netlist("network", 1000.0, gm=0.0, analysis=ctl).replace("Vin mix 0 dc 0 ac 1", "Vin mix 0 dc 1 ac 1")
    print(net)
    print(run(net, wd, "diag"))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--diag", action="store_true")
    ap.add_argument("--out", default="sim-out")
    ap.add_argument("--shift", type=float, default=248.0 / 227.9, help="plugin selfOscShift")
    ap.add_argument("--kmax", type=float, default=4.05)
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)
    wd = tempfile.mkdtemp(prefix="ir3109_")
    if args.diag:
        diag(wd)
        return

    freqs = [100.0, 1000.0, 4000.0, 8000.0]
    g = ac_gain_db(wd, "model", 1000.0, freqs, k=0.0)
    with open(os.path.join(args.out, "response.csv"), "w", newline="") as f:
        w = csv.writer(f); w.writerow(["freqHz", "gainDb"])
        for x, y in zip(freqs, g): w.writerow([x, f"{y:.3f}"])

    fc = 248.0 * args.shift
    # 3 s: the oscillation needs well over a second to settle at its saturated level.
    t, y = tran(wd, "model", fc, tstop=3.0, tstep=1.0 / 96000, k=args.kmax, amp=0.0, freq=248.0)
    sel = t > 2.0
    fosc = dominant(t[sel], y[sel], 150.0, 400.0)
    vpp = float(y[sel].max() - y[sel].min())
    with open(os.path.join(args.out, "selfosc.csv"), "w", newline="") as f:
        w = csv.writer(f); w.writerow(["cornerHz", "k", "oscHz", "vpp"]); w.writerow([f"{fc:.3f}", args.kmax, f"{fosc:.3f}", f"{vpp:.4f}"])

    t, y = tran(wd, "model", 2000.0, tstop=0.2, tstep=1.0 / 192000, k=0.0, amp=6.0, freq=220.0)
    sel = t > 0.1
    h = harmonic_db(t[sel], y[sel], 220.0, 5)
    with open(os.path.join(args.out, "harmonics.csv"), "w", newline="") as f:
        w = csv.writer(f); w.writerow(["harmonic", "dbReH1"])
        for i, v in enumerate(h, start=2): w.writerow([i, f"{v:.2f}"])

    rows = []
    for k in [0.0, 1.0, 2.0, 3.0, 3.9]:
        rows.append(["model", f"{k:.2f}", f"{ac_gain_db(wd, 'model', 1000.0, [100.0], k=k)[0]:.3f}"])

    # Network as drawn: find the oscillation threshold GM_osc by bisection on transient growth,
    # then report the passband on the normalised axis k = 4 GM / GM_osc.
    def grows(gm):
        t, y = tran(wd, "network", 1000.0, tstop=0.06, tstep=1.0 / 192000, gm=gm, amp=0.0, freq=100.0)
        # The kick injects about 40 uV; a decaying ring is far below 1 mV by the last quarter,
        # a sustained or saturated oscillation is far above it.
        a = np.abs(y - y.mean())
        q = len(a) // 4
        return a[3 * q:].max() > 1e-3
    lo, hi = 0.0, 0.1
    for _ in range(14):
        mid = 0.5 * (lo + hi)
        if grows(mid): hi = mid
        else: lo = mid
    gm_osc = 0.5 * (lo + hi)
    for kn in [0.0, 1.0, 2.0, 3.0, 3.9]:
        gm = gm_osc * kn / 4.0
        rows.append(["network", f"{kn:.2f}", f"{ac_gain_db(wd, 'network', 1000.0, [100.0], gm=gm)[0]:.3f}"])
    t, y = tran(wd, "network", fc, tstop=3.0, tstep=1.0 / 96000, gm=gm_osc * args.kmax / 4.0, amp=0.0, freq=248.0)
    sel = t > 2.0
    with open(os.path.join(args.out, "network.csv"), "w", newline="") as f:
        w = csv.writer(f); w.writerow(["gmOsc", "cornerHz", "k", "oscHz", "vpp"])
        w.writerow([f"{gm_osc:.6f}", f"{fc:.3f}", args.kmax, f"{dominant(t[sel], y[sel], 150.0, 400.0):.3f}", f"{float(y[sel].max() - y[sel].min()):.4f}"])
    with open(os.path.join(args.out, "passband.csv"), "w", newline="") as f:
        w = csv.writer(f); w.writerow(["mode", "k_or_gm", "gain100HzDb"]); w.writerows(rows)
    print("wrote", args.out)

if __name__ == "__main__":
    main()

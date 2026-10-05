// SPDX-License-Identifier: GPL-3.0-or-later
// Runs the plugin's IR3109 block through the same measurements as the ngspice
// reference (tools/sim/ir3109_ref.py) and writes the same CSV files.
#include "dsp/Calibration.h"
#include "dsp/vcf/Ir3109.h"
#include "dsp/voice/Voice.h"

#include <cmath>
#include <complex>
#include <cstdio>
#include <string>
#include <vector>

using namespace jane60;

namespace
{
constexpr double kPi = 3.14159265358979323846;

double gainDb (const Calibration& cal, double sr, double fc, double k, double fIn)
{
    Ir3109 f;
    f.configure (cal.vcf, sr);
    f.set (fc, k);
    const double amp = 0.01;
    double re = 0.0, im = 0.0;
    const int n = static_cast<int> (sr), skip = n / 2;
    for (int i = 0; i < n; ++i)
    {
        const double ph = 2.0 * kPi * fIn * i / sr;
        const double y = f.process (amp * std::sin (ph));
        if (i >= skip) { re += y * std::cos (ph); im += y * std::sin (ph); }
    }
    const double mag = 2.0 * std::hypot (re, im) / (n - skip);
    return 20.0 * std::log10 (mag / amp);
}

void write (const std::string& dir, const char* name, const std::string& text)
{
    FILE* f = std::fopen ((dir + "/" + name).c_str(), "w");
    if (f == nullptr) { std::perror (name); std::exit (1); }
    std::fputs (text.c_str(), f);
    std::fclose (f);
}
} // namespace

int main (int argc, char** argv)
{
    if (argc < 3) { std::fprintf (stderr, "usage: %s calibration.json outdir\n", argv[0]); return 1; }
    const auto cal = Calibration::fromFile (argv[1]);
    const std::string out = argv[2];
    const double sr = 48000.0; // the usual host rate; the block runs 2x inside (its behaviour is rate dependent, see the report)
    char buf[256];

    std::string s = "freqHz,gainDb\n";
    for (double fIn : { 100.0, 1000.0, 4000.0, 8000.0 })
    {
        std::snprintf (buf, sizeof buf, "%g,%.3f\n", fIn, gainDb (cal, sr, 1000.0, 0.0, fIn));
        s += buf;
    }
    write (out, "response.csv", s);

    {
        VcfMapping map;
        Ir3109 f;
        f.configure (cal.vcf, sr);
        const double fc = 248.0 * map.selfOscShift;
        f.set (fc, map.kMax);
        std::vector<double> y;
        // 3 s: the oscillation needs well over a second to settle at its saturated level.
        for (int i = 0; i < static_cast<int> (3.0 * sr); ++i)
            y.push_back (f.process (i < 10 ? 0.05 : 0.0));
        const std::size_t from = y.size() * 2 / 3;
        // Dominant frequency by DFT peak search in 150..400 Hz.
        double best = 0.0, bestF = 0.0;
        const auto n = static_cast<double> (y.size() - from);
        for (double fr = 150.0; fr <= 400.0; fr += 0.1)
        {
            double re = 0.0, im = 0.0;
            for (std::size_t i = from; i < y.size(); ++i)
            {
                const double ph = 2.0 * kPi * fr * static_cast<double> (i) / sr;
                re += y[i] * std::cos (ph);
                im += y[i] * std::sin (ph);
            }
            const double m = std::hypot (re, im) / n;
            if (m > best) { best = m; bestF = fr; }
        }
        double mx = -1e9, mn = 1e9;
        for (std::size_t i = from; i < y.size(); ++i) { mx = std::max (mx, y[i]); mn = std::min (mn, y[i]); }
        std::snprintf (buf, sizeof buf, "cornerHz,k,oscHz,vpp\n%.3f,%g,%.3f,%.4f\n", fc, map.kMax, bestF, mx - mn);
        write (out, "selfosc.csv", buf);
    }

    {
        Ir3109 f;
        f.configure (cal.vcf, sr);
        f.set (2000.0, 0.0);
        const int n = static_cast<int> (sr) / 5, skip = n / 2;
        std::vector<double> y;
        for (int i = 0; i < n; ++i)
            y.push_back (f.process (6.0 * std::sin (2.0 * kPi * 220.0 * i / sr)));
        double h[6] {};
        for (int hn = 1; hn <= 5; ++hn)
        {
            double re = 0.0, im = 0.0;
            for (int i = skip; i < n; ++i)
            {
                const double ph = 2.0 * kPi * hn * 220.0 * i / sr;
                re += y[static_cast<std::size_t> (i)] * std::cos (ph);
                im += y[static_cast<std::size_t> (i)] * std::sin (ph);
            }
            h[hn] = std::hypot (re, im);
        }
        s = "harmonic,dbReH1\n";
        for (int hn = 2; hn <= 5; ++hn)
        {
            std::snprintf (buf, sizeof buf, "%d,%.2f\n", hn, 20.0 * std::log10 (h[hn] / h[1] + 1e-15));
            s += buf;
        }
        write (out, "harmonics.csv", s);
    }

    s = "mode,k_or_gm,gain100HzDb\n";
    for (double k : { 0.0, 1.0, 2.0, 3.0, 3.9 })
    {
        std::snprintf (buf, sizeof buf, "model,%.2f,%.3f\n", k, gainDb (cal, sr, 1000.0, k, 100.0));
        s += buf;
    }
    write (out, "passband.csv", s);
    std::printf ("wrote %s\n", out.c_str());
    return 0;
}

// SPDX-License-Identifier: GPL-3.0-or-later
// Renders every factory patch through the whole synth (voices, HPF, level, chorus,
// output stage) playing a fixed phrase, as stereo float32 files for the listening
// comparison in tools/listen/compare_reference.py.
#include "dsp/Synth.h"
#include "dsp/presets/FactoryPatches.h"

#include <cstdio>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace jane60;

#include <map>

int main (int argc, char** argv)
{
    if (argc < 4) { std::fprintf (stderr, "usage: %s calibration.json factory.csv outdir [phrases.txt]\n", argv[0]); return 1; }
    const auto cal = Calibration::fromFile (argv[1]);
    std::ifstream in (argv[2]);
    std::stringstream ss;
    ss << in.rdbuf();
    const auto patches = parseFactoryPatches (ss.str());
    const std::string out = argv[3];
    constexpr double sr = 48000.0;
    constexpr int block = 256;

    // Default phrase (seconds): C3 E3 G3 chord held 0..2.5, then C4 single 3..4.5, then C2 bass 5..6.5.
    struct Ev { double t; bool on; int note; };
    const std::vector<Ev> defaultPhrase = {
        { 0.0, true, 48 }, { 0.0, true, 52 }, { 0.0, true, 55 }, { 2.5, false, 48 }, { 2.5, false, 52 }, { 2.5, false, 55 },
        { 3.0, true, 60 }, { 4.5, false, 60 },
        { 5.0, true, 36 }, { 6.5, false, 36 } };
    // Optional per-patch phrases: lines "patch start note duration" (seconds, MIDI note).
    std::map<int, std::vector<Ev>> phrases;
    if (argc >= 5)
    {
        std::ifstream pf (argv[4]);
        int num, note;
        double start, dur;
        while (pf >> num >> start >> note >> dur)
        {
            phrases[num].push_back ({ start, true, note });
            phrases[num].push_back ({ start + dur, false, note });
        }
    }

    for (const auto& fp : patches)
    {
        const auto it = phrases.find (fp.number);
        const std::vector<Ev>& phrase = it != phrases.end() ? it->second : defaultPhrase;
        double total = 0.0;
        for (const auto& e : phrase) total = std::max (total, e.t);
        total += 2.5; // release tail
        Synth s;
        s.setPanel (fp.panel);
        s.prepare (cal, sr, 440.0);
        std::vector<float> l (block), r (block), inter;
        inter.reserve (static_cast<std::size_t> (total * sr) * 2);
        const int n = static_cast<int> (total * sr);
        for (int pos = 0; pos < n; pos += block)
        {
            std::vector<MidiEvent> ev;
            for (const auto& e : phrase)
            {
                const int at = static_cast<int> (e.t * sr);
                if (at >= pos && at < pos + block)
                    ev.push_back ({ at - pos, e.on ? MidiEvent::Type::noteOn : MidiEvent::Type::noteOff, e.note, 0.0 });
            }
            s.render (l.data(), r.data(), block, ev);
            for (int i = 0; i < block; ++i) { inter.push_back (l[static_cast<std::size_t> (i)]); inter.push_back (r[static_cast<std::size_t> (i)]); }
        }
        char num[16];
        std::snprintf (num, sizeof num, "%02d", fp.number);
        const std::string name = out + "/patch_" + num + ".f32";
        FILE* f = std::fopen (name.c_str(), "wb");
        if (f == nullptr) { std::perror (name.c_str()); return 1; }
        std::fwrite (inter.data(), sizeof (float), inter.size(), f);
        std::fclose (f);
    }
    std::printf ("rendered %zu patches to %s\n", patches.size(), out.c_str());
    return 0;
}

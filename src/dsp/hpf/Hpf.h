// SPDX-License-Identifier: GPL-3.0-or-later
// The shared HPF on the chorus board: one RC pole, four switched positions,
// position 0 flat (Owner's Manual p.14; Service Notes p.11).
#pragma once

#include "dsp/Calibration.h"

#include <cmath>

namespace jane60
{

class Hpf
{
public:
    void configure (const Calibration::Hpf& c, double sampleRate) noexcept
    {
        constexpr double pi = 3.14159265358979323846;
        for (int i = 0; i < 4; ++i)
        {
            const double fc = c.cornerHz[static_cast<std::size_t> (i)];
            if (fc <= 0.0)
                a_[i] = 1.0; // bypass
            else
            {
                const double g = std::tan (pi * fc / sampleRate);
                a_[i] = g / (1.0 + g);
            }
        }
        setPosition (0);
        s_ = 0.0;
    }

    void setPosition (int pos) noexcept
    {
        if (pos < 0) pos = 0;
        if (pos > 3) pos = 3;
        pos_ = pos;
    }

    double process (double x) noexcept
    {
        if (pos_ == 0) return x;
        // TPT one-pole high-pass: y = x - lp
        const double G = a_[pos_];
        const double v = (x - s_) * G;
        const double lp = v + s_;
        s_ = lp + v;
        return x - lp;
    }

private:
    double a_[4] { 1.0, 0.0, 0.0, 0.0 };
    int pos_ = 0;
    double s_ = 0.0;
};

} // namespace jane60

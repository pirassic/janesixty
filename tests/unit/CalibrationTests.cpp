// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsp/Calibration.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

using namespace jane60;
using Catch::Matchers::WithinRel;

static Calibration load()
{
    return Calibration::fromFile (JANE60_CALIBRATION_FILE);
}

TEST_CASE ("calibration file parses and reports schema 1")
{
    const auto c = load();
    CHECK (c.schema == 1);
    CHECK (c.firmware.voices == 6);
    CHECK (c.firmware.sliderBits == 8);
}

TEST_CASE ("service-notes anchors are present with their values")
{
    const auto c = load();
    CHECK_THAT (c.clock.masterClockHz, WithinRel (1902810.0, 1e-9));
    CHECK_THAT (c.clock.tuningA4Hz, WithinRel (442.0, 1e-9));
    CHECK_THAT (c.vcf.anchorHz, WithinRel (248.0, 1e-9));
    CHECK (c.vcf.keyFollowPivotNote == 60);
    CHECK_THAT (c.vcf.envFullPeakHz, WithinRel (30000.0, 1e-9));
    CHECK_THAT (c.dco.sawVpp, WithinRel (12.0, 1e-9));
    CHECK_THAT (c.env.attackSeconds.values.back(), WithinRel (3.0, 1e-9));
    CHECK_THAT (c.lfo.rateHz.values.back(), WithinRel (22.0, 1e-9));
}

TEST_CASE ("8253 divisor reproduces the service-notes worked example")
{
    // Service Notes p.14: master 1902810 Hz, divisor 4305 -> 442 Hz.
    const auto c = load();
    const auto divisor = std::lround (c.clock.masterClockHz / c.clock.tuningA4Hz);
    CHECK (divisor == 4305);
}

TEST_CASE ("slider tables are monotonic where the hardware is")
{
    const auto c = load();
    for (std::size_t i = 1; i < 11; ++i)
    {
        CHECK (c.env.attackSeconds.values[i] > c.env.attackSeconds.values[i - 1]);
        CHECK (c.env.decaySeconds.values[i] > c.env.decaySeconds.values[i - 1]);
        CHECK (c.lfo.rateHz.values[i] > c.lfo.rateHz.values[i - 1]);
        CHECK (c.env.sustainLevel.values[i] >= c.env.sustainLevel.values[i - 1]);
    }
    CHECK (c.hpf.cornerHz[0] == 0.0);
    CHECK (c.hpf.cornerHz[1] < c.hpf.cornerHz[2]);
    CHECK (c.hpf.cornerHz[2] < c.hpf.cornerHz[3]);
}

TEST_CASE ("slider table interpolation")
{
    SliderTable t;
    for (std::size_t i = 0; i < 11; ++i)
        t.values[i] = static_cast<double> (i) * 2.0;
    CHECK_THAT (t.at (0.0), WithinRel (0.0, 1e-12));
    CHECK_THAT (t.at (2.5), WithinRel (5.0, 1e-12));
    CHECK_THAT (t.at (10.0), WithinRel (20.0, 1e-12));
    CHECK_THAT (t.at (-1.0), WithinRel (0.0, 1e-12));
    CHECK_THAT (t.at (11.0), WithinRel (20.0, 1e-12));
}

TEST_CASE ("chorus modes match the measured table")
{
    const auto c = load();
    REQUIRE (c.chorus.modes.size() == 3);
    CHECK (c.chorus.modes[0].name == "I");
    CHECK_THAT (c.chorus.modes[0].lfoRateHz, WithinRel (0.513, 1e-9));
    CHECK (c.chorus.modes[0].stereo);
    CHECK (c.chorus.modes[2].name == "I+II");
    CHECK_FALSE (c.chorus.modes[2].stereo);
}

TEST_CASE ("every constant carries a source tag and assumed ones are listed")
{
    const auto c = load();
    CHECK (c.sources.size() >= 50);
    const auto assumed = c.assumedKeys();
    CHECK_FALSE (assumed.empty());
    for (const auto& k : assumed)
        CHECK (c.sources.at (k).rfind ("assumed", 0) == 0);
}

TEST_CASE ("malformed input is rejected with a message")
{
    CHECK_THROWS_AS (Calibration::fromJson ("{"), std::runtime_error);
    CHECK_THROWS_AS (Calibration::fromJson ("{\"schema\": 2}"), std::runtime_error);
    CHECK_THROWS_AS (Calibration::fromJson ("{\"schema\": 1}"), std::runtime_error);
}

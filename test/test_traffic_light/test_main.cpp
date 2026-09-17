#include <unity.h>
#include "core/TrafficLight.h"
#include "../support/Fakes.h"

void setUp() {}
void tearDown() {}

void assertLamp(Lamp expected, const FakeLedOutputs& outputs) {
    const bool yellow = expected == Lamp::Yellow;
    const bool green = expected == Lamp::Green;
    for (std::size_t i = 0; i < outputs.frame.size(); ++i) {
        TEST_ASSERT_EQUAL_UINT8(((i == 0 && expected == Lamp::Red) || (i == 1 && yellow)) ? 255 : 0,
                                outputs.frame[i].red);
        TEST_ASSERT_EQUAL_UINT8(((i == 1 && yellow) || (i == 2 && green)) ? 255 : 0,
                                outputs.frame[i].green);
        TEST_ASSERT_EQUAL_UINT8(0, outputs.frame[i].blue);
    }
}

bool sameFrame(const LedFrame& left, const LedFrame& right) {
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (left[i].red != right[i].red || left[i].green != right[i].green ||
            left[i].blue != right[i].blue) return false;
    }
    return true;
}

void assertSmoothStep(const LedFrame& before, const LedFrame& after) {
    for (std::size_t i = 0; i < before.size(); ++i) {
        const uint8_t previous[] = {before[i].red, before[i].green, before[i].blue};
        const uint8_t current[] = {after[i].red, after[i].green, after[i].blue};
        for (std::size_t channel = 0; channel < 3; ++channel) {
            const int difference = static_cast<int>(current[channel]) - previous[channel];
            TEST_ASSERT_LESS_OR_EQUAL(3, difference < 0 ? -difference : difference);
        }
    }
}

void rgb_pairs_yellow_mixing_and_off() {
    FakeClock clock; FakeLedOutputs outputs; TrafficLight lights(clock, outputs);
    TEST_ASSERT_TRUE(lights.begin());
    assertLamp(Lamp::Off, outputs);
    for (auto lamp : {Lamp::Red, Lamp::Yellow, Lamp::Green, Lamp::Off}) {
        lights.show(lamp); assertLamp(lamp, outputs);
    }
    lights.show(static_cast<Lamp>(99)); assertLamp(Lamp::Off, outputs);
}

void animation_fades_smoothly_and_stops_on_lamp() {
    FakeClock clock; FakeLedOutputs outputs; TrafficLight lights(clock, outputs);
    TEST_ASSERT_TRUE(lights.begin());
    lights.show(Lamp::Green);
    lights.seedAnimation(0x12345678);
    lights.startAnimation();
    const auto startingFrame = outputs.frame;
    clock.advance(Config::AnimationFrameMs - 1); lights.updateAnimation();
    TEST_ASSERT_EQUAL(3, outputs.writes);
    clock.advance(1); lights.updateAnimation();
    TEST_ASSERT_FALSE(sameFrame(startingFrame, outputs.frame));
    assertSmoothStep(startingFrame, outputs.frame);
    for (unsigned i = 0; i < 50; ++i) {
        const auto before = outputs.frame;
        clock.advance(Config::AnimationFrameMs);
        lights.updateAnimation();
        assertSmoothStep(before, outputs.frame);
    }
    lights.show(Lamp::Yellow);
    const int written = outputs.writes;
    clock.advance(Config::AnimationMaxFadeMs); lights.updateAnimation();
    TEST_ASSERT_EQUAL(written, outputs.writes);
    assertLamp(Lamp::Yellow, outputs);
}

void seeded_animation_is_repeatable_and_each_run_changes() {
    FakeClock firstClock, secondClock, otherClock;
    FakeLedOutputs firstOutputs, secondOutputs, otherOutputs;
    TrafficLight first(firstClock, firstOutputs), second(secondClock, secondOutputs), other(otherClock, otherOutputs);
    for (auto* light : {&first, &second, &other}) {
        TEST_ASSERT_TRUE(light->begin());
        light->show(Lamp::Green);
    }
    first.seedAnimation(1234); second.seedAnimation(1234); other.seedAnimation(5678);
    first.startAnimation(); second.startAnimation(); other.startAnimation();
    for (unsigned i = 0; i < 100; ++i) {
        firstClock.advance(Config::AnimationFrameMs);
        secondClock.advance(Config::AnimationFrameMs);
        otherClock.advance(Config::AnimationFrameMs);
        first.updateAnimation(); second.updateAnimation(); other.updateAnimation();
        TEST_ASSERT_TRUE(sameFrame(firstOutputs.frame, secondOutputs.frame));
    }
    TEST_ASSERT_FALSE(sameFrame(firstOutputs.frame, otherOutputs.frame));

    const auto previousRun = firstOutputs.frame;
    first.show(Lamp::Green);
    firstClock.advance(137);
    first.startAnimation();
    for (unsigned i = 0; i < 100; ++i) {
        firstClock.advance(Config::AnimationFrameMs);
        first.updateAnimation();
    }
    TEST_ASSERT_FALSE(sameFrame(previousRun, firstOutputs.frame));
}

void random_targets_cover_the_rgb_spectrum_and_rollover() {
    FakeClock clock; FakeLedOutputs outputs; TrafficLight lights(clock, outputs);
    TEST_ASSERT_TRUE(lights.begin());
    lights.seedAnimation(0);
    clock.time = UINT32_MAX - 10;
    lights.startAnimation();
    clock.advance(Config::AnimationFrameMs);
    lights.updateAnimation();
    TEST_ASSERT_GREATER_THAN(1, outputs.writes);

    bool redDominant = false, greenDominant = false, blueDominant = false;
    for (unsigned transition = 0; transition < 100; ++transition) {
        clock.advance(Config::AnimationMaxFadeMs);
        lights.updateAnimation();
        for (const auto& color : outputs.frame) {
            TEST_ASSERT_TRUE(color.red || color.green || color.blue);
            redDominant |= color.red > color.green && color.red > color.blue;
            greenDominant |= color.green > color.red && color.green > color.blue;
            blueDominant |= color.blue > color.red && color.blue > color.green;
        }
    }
    TEST_ASSERT_TRUE(redDominant);
    TEST_ASSERT_TRUE(greenDominant);
    TEST_ASSERT_TRUE(blueDominant);
}

void invalid_interval_inactive_calls_and_adapter_failures() {
    FakeClock clock; FakeLedOutputs outputs; TrafficLight lights(clock, outputs);
    lights.show(Lamp::Red); lights.startAnimation(); lights.updateAnimation();
    TEST_ASSERT_EQUAL(0, outputs.writes);
    TrafficLight zero(clock, outputs, 0);
    TrafficLight tooSlow(clock, outputs, Config::AnimationMinFadeMs + 1);
    TEST_ASSERT_FALSE(zero.begin());
    TEST_ASSERT_FALSE(tooSlow.begin());
    TEST_ASSERT_EQUAL(0, outputs.begins);
    outputs.ok = false; TEST_ASSERT_FALSE(lights.begin());
    lights.show(Lamp::Red); lights.startAnimation(); lights.updateAnimation();
    TEST_ASSERT_EQUAL(0, outputs.writes);
    outputs.ok = true; TEST_ASSERT_TRUE(lights.begin()); lights.startAnimation();
    outputs.ok = false; TEST_ASSERT_FALSE(lights.begin()); const int written = outputs.writes;
    clock.advance(Config::AnimationFrameMs); lights.updateAnimation();
    TEST_ASSERT_EQUAL(written, outputs.writes);
    outputs.ok = true; TEST_ASSERT_TRUE(lights.begin()); assertLamp(Lamp::Off, outputs);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(rgb_pairs_yellow_mixing_and_off);
    RUN_TEST(animation_fades_smoothly_and_stops_on_lamp);
    RUN_TEST(seeded_animation_is_repeatable_and_each_run_changes);
    RUN_TEST(random_targets_cover_the_rgb_spectrum_and_rollover);
    RUN_TEST(invalid_interval_inactive_calls_and_adapter_failures);
    return UNITY_END();
}

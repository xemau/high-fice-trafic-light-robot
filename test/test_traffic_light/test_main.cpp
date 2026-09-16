#include <unity.h>
#include "core/TrafficLight.h"
#include "../support/Fakes.h"
void setUp() {}
void tearDown() {}
void assertLamp(Lamp expected, const FakeLedOutputs& outputs) {
    for (std::size_t i = 0; i < outputs.frame.size(); ++i) {
        TEST_ASSERT_EQUAL((i == 0 && expected == Lamp::Red) || (i == 1 && expected == Lamp::Yellow), outputs.frame[i].red);
        TEST_ASSERT_EQUAL((i == 1 && expected == Lamp::Yellow) || (i == 2 && expected == Lamp::Green), outputs.frame[i].green);
        TEST_ASSERT_FALSE(outputs.frame[i].blue);
    }
}
void assertAnimation(unsigned phase, const FakeLedOutputs& outputs) {
    for (std::size_t i = 0; i < outputs.frame.size(); ++i) {
        TEST_ASSERT_EQUAL((i + phase) % 3 == 0, outputs.frame[i].red);
        TEST_ASSERT_EQUAL((i + phase) % 3 == 1, outputs.frame[i].green);
        TEST_ASSERT_EQUAL((i + phase) % 3 == 2, outputs.frame[i].blue);
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
void animation_boundary_progression_and_stop() {
    FakeClock clock; FakeLedOutputs outputs; TrafficLight lights(clock, outputs);
    lights.begin(); lights.updateAnimation(); TEST_ASSERT_EQUAL(1, outputs.writes);
    lights.startAnimation(); TEST_ASSERT_EQUAL(2, outputs.writes); assertAnimation(0, outputs);
    clock.advance(Config::AnimationMs - 1); lights.updateAnimation(); TEST_ASSERT_EQUAL(2, outputs.writes);
    clock.advance(1); lights.updateAnimation(); assertAnimation(1, outputs);
    clock.advance(Config::AnimationMs + 1); lights.updateAnimation(); assertAnimation(2, outputs);
    clock.advance(Config::AnimationMs); lights.updateAnimation(); assertAnimation(0, outputs);
    for (auto lamp : {Lamp::Red, Lamp::Yellow, Lamp::Green, Lamp::Off}) {
        lights.startAnimation(); lights.show(lamp); const int written = outputs.writes;
        clock.advance(1000); lights.updateAnimation();
        TEST_ASSERT_EQUAL(written, outputs.writes); assertLamp(lamp, outputs);
    }
    lights.startAnimation(); clock.advance(Config::AnimationMs); lights.updateAnimation();
    lights.startAnimation(); assertAnimation(0, outputs);
}
void animation_rollover_and_bounded_catchup() {
    FakeClock clock; FakeLedOutputs outputs; TrafficLight lights(clock, outputs);
    lights.begin(); clock.time = UINT32_MAX - 20; lights.startAnimation();
    clock.advance(Config::AnimationMs); lights.updateAnimation(); assertAnimation(1, outputs);
    const int written = outputs.writes;
    clock.advance(Config::AnimationMs * 1000000); lights.updateAnimation();
    TEST_ASSERT_EQUAL(written + 1, outputs.writes); assertAnimation(2, outputs);
}
void invalid_interval_and_adapter_failures() {
    FakeClock clock; FakeLedOutputs outputs; TrafficLight lights(clock, outputs);
    lights.show(Lamp::Red); lights.startAnimation(); lights.updateAnimation();
    TEST_ASSERT_EQUAL(0, outputs.writes);
    TrafficLight zero(clock, outputs, 0); TEST_ASSERT_FALSE(zero.begin());
    TEST_ASSERT_EQUAL(0, outputs.begins);
    outputs.ok = false; TEST_ASSERT_FALSE(lights.begin());
    lights.show(Lamp::Red); lights.startAnimation(); lights.updateAnimation();
    TEST_ASSERT_EQUAL(0, outputs.writes);
    outputs.ok = true; TEST_ASSERT_TRUE(lights.begin()); lights.startAnimation();
    outputs.ok = false; TEST_ASSERT_FALSE(lights.begin()); const int written = outputs.writes;
    clock.advance(Config::AnimationMs); lights.updateAnimation(); TEST_ASSERT_EQUAL(written, outputs.writes);
    outputs.ok = true; TEST_ASSERT_TRUE(lights.begin()); assertLamp(Lamp::Off, outputs);
}
int main() {
    UNITY_BEGIN(); RUN_TEST(rgb_pairs_yellow_mixing_and_off); RUN_TEST(animation_boundary_progression_and_stop);
    RUN_TEST(animation_rollover_and_bounded_catchup); RUN_TEST(invalid_interval_and_adapter_failures);
    return UNITY_END();
}

#include <unity.h>
#include "core/HighFiveSensor.h"
#include "../support/Fakes.h"
void setUp() {}
void tearDown() {}
struct Rig {
    FakeClock clock; FakeInput input; HighFiveSensor sensor{clock, input};
    Rig() { sensor.begin(); }
    void level(bool high) { input.level = high; sensor.update(); }
    void wait(uint32_t ms) { clock.advance(ms); sensor.update(); }
};
void released_and_short_press() {
    Rig r; TEST_ASSERT_FALSE(r.sensor.pressed()); TEST_ASSERT_FALSE(r.sensor.takePress());
    r.level(false); r.wait(29); TEST_ASSERT_FALSE(r.sensor.takePress());
    r.level(true); r.wait(30); TEST_ASSERT_FALSE(r.sensor.pressed()); TEST_ASSERT_FALSE(r.sensor.takePress());
}
void stable_press_hold_release_second_press() {
    Rig r; r.level(false); r.wait(29); TEST_ASSERT_FALSE(r.sensor.pressed());
    r.wait(1); TEST_ASSERT_TRUE(r.sensor.pressed()); TEST_ASSERT_TRUE(r.sensor.takePress());
    r.wait(1); TEST_ASSERT_FALSE(r.sensor.takePress());
    r.wait(1000); TEST_ASSERT_FALSE(r.sensor.takePress());
    r.level(true); r.wait(29); TEST_ASSERT_TRUE(r.sensor.pressed());
    r.wait(1); TEST_ASSERT_FALSE(r.sensor.pressed()); TEST_ASSERT_FALSE(r.sensor.takePress());
    r.level(false); r.wait(31); TEST_ASSERT_TRUE(r.sensor.takePress());
}
void bounce_restarts_debounce() {
    Rig r;
    for (int i = 0; i < 10; ++i) { r.level(false); r.wait(10); r.level(true); r.wait(10); }
    TEST_ASSERT_FALSE(r.sensor.takePress());
    r.level(false); r.wait(30); TEST_ASSERT_TRUE(r.sensor.takePress());
    r.level(true); r.wait(10); r.level(false); r.wait(30);
    TEST_ASSERT_TRUE(r.sensor.pressed()); TEST_ASSERT_FALSE(r.sensor.takePress());
}
void startup_held_is_not_a_highfive() {
    Rig r; r.input.level = false; r.sensor.begin(); r.wait(100);
    TEST_ASSERT_TRUE(r.sensor.pressed()); TEST_ASSERT_FALSE(r.sensor.takePress());
    r.level(true); r.wait(30); r.level(false); r.wait(30); TEST_ASSERT_TRUE(r.sensor.takePress());
}
void rollover_debounce_and_pending_event() {
    Rig r; r.clock.time = UINT32_MAX - 10; r.level(false); r.wait(29);
    TEST_ASSERT_FALSE(r.sensor.takePress()); r.wait(1);
    r.level(true); r.wait(30); TEST_ASSERT_TRUE(r.sensor.takePress()); TEST_ASSERT_FALSE(r.sensor.takePress());
}
void failed_input_and_zero_debounce() {
    Rig r; r.input.ok = false; TEST_ASSERT_FALSE(r.sensor.begin());
    r.level(false); r.wait(100); TEST_ASSERT_FALSE(r.sensor.pressed()); TEST_ASSERT_FALSE(r.sensor.takePress());
    r.input.ok = true; r.input.level = true;
    HighFiveSensor immediate(r.clock, r.input, 0); immediate.begin();
    r.input.level = false; immediate.update(); TEST_ASSERT_TRUE(immediate.takePress());
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(released_and_short_press); RUN_TEST(stable_press_hold_release_second_press);
    RUN_TEST(bounce_restarts_debounce); RUN_TEST(startup_held_is_not_a_highfive);
    RUN_TEST(rollover_debounce_and_pending_event); RUN_TEST(failed_input_and_zero_debounce);
    return UNITY_END();
}

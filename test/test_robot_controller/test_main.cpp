#include <unity.h>
#include "core/RobotController.h"
#include "core/HighFiveSensor.h"
#include "../support/Fakes.h"

void setUp() {}
void tearDown() {}
struct Rig {
    FakeClock clock;
    FakeSensor sensor;
    FakeAudio audio;
    FakeLights lights;
    FakeLog log;
    RobotController robot{clock, sensor, audio, lights, log};
    Rig() { robot.begin(); }
    void green() { clock.advance(Config::RedMs); robot.update(); clock.advance(Config::YellowMs); robot.update(); }
    void reward() { green(); sensor.event = true; robot.update(); }
};
void initial_red_and_inactive_update() {
    Rig r;
    TEST_ASSERT_EQUAL_INT(RobotState::Red, r.robot.state());
    TEST_ASSERT_EQUAL_INT(Lamp::Red, r.lights.lamp);
    TEST_ASSERT_TRUE(r.log.contains("[STATE] RED"));
    RobotController inactive(r.clock, r.sensor, r.audio, r.lights, r.log);
    inactive.update();
    TEST_ASSERT_EQUAL(0, r.sensor.updates);
}
void red_and_yellow_boundaries() {
    TEST_ASSERT_EQUAL_UINT32(3000, Config::RedMs);
    for (uint32_t delta : {0u, 1u}) {
        Rig r;
        r.clock.advance(Config::RedMs - 1); r.robot.update();
        TEST_ASSERT_EQUAL_INT(RobotState::Red, r.robot.state());
        r.clock.advance(1 + delta); r.robot.update();
        TEST_ASSERT_EQUAL_INT(RobotState::Yellow, r.robot.state());
        r.clock.advance(Config::YellowMs - 1); r.robot.update();
        TEST_ASSERT_EQUAL_INT(RobotState::Yellow, r.robot.state());
        r.clock.advance(1 + delta); r.robot.update();
        TEST_ASSERT_EQUAL_INT(RobotState::GreenWaiting, r.robot.state());
        TEST_ASSERT_EQUAL_INT(Lamp::Green, r.lights.lamp);
    }
}
void green_waits_indefinitely() {
    Rig r; r.green();
    r.clock.advance(1000000000); r.robot.update();
    TEST_ASSERT_EQUAL_INT(RobotState::GreenWaiting, r.robot.state());
    TEST_ASSERT_EQUAL(0, r.audio.plays);
}
void early_events_are_consumed_not_deferred() {
    Rig r;
    r.sensor.event = true; r.robot.update();
    r.clock.advance(Config::RedMs); r.sensor.event = true; r.robot.update();
    r.sensor.event = true; r.robot.update();
    r.clock.advance(Config::YellowMs); r.sensor.event = true; r.robot.update();
    r.robot.update();
    TEST_ASSERT_EQUAL_INT(RobotState::GreenWaiting, r.robot.state());
    TEST_ASSERT_EQUAL(0, r.audio.plays);
}
void reward_once_then_red_at_boundary() {
    TEST_ASSERT_EQUAL_UINT32(30000, Config::RewardMs);
    TEST_ASSERT_EQUAL_UINT32(26000, Config::RewardWarningMs);
    for (uint32_t start : {0u, UINT32_MAX - 15000}) for (uint32_t delta : {0u, 1u}) {
        Rig r; r.clock.time = start; r.reward();
        TEST_ASSERT_EQUAL_INT(RobotState::Reward, r.robot.state());
        TEST_ASSERT_GREATER_OR_EQUAL(Config::RewardTrack, r.audio.track);
        TEST_ASSERT_LESS_THAN(Config::RewardTrack + Config::RewardTrackCount, r.audio.track);
        TEST_ASSERT_TRUE(r.lights.dancing);
        r.sensor.down = true;
        r.robot.update(); r.robot.simulateHighFive(); r.robot.update();
        TEST_ASSERT_EQUAL(1, r.audio.plays);
        TEST_ASSERT_EQUAL(1, r.lights.animations);
        r.clock.advance(Config::RewardWarningMs - 1); r.robot.update();
        TEST_ASSERT_EQUAL_INT(RobotState::Reward, r.robot.state());
        TEST_ASSERT_EQUAL(0, r.audio.stops);
        TEST_ASSERT_TRUE(r.lights.dancing);
        r.clock.advance(1); r.robot.update();
        TEST_ASSERT_EQUAL_INT(RobotState::Reward, r.robot.state());
        TEST_ASSERT_EQUAL_INT(Lamp::Yellow, r.lights.lamp);
        TEST_ASSERT_FALSE(r.lights.dancing);
        TEST_ASSERT_EQUAL(0, r.audio.stops);
        r.clock.advance(Config::RewardMs - Config::RewardWarningMs - 1); r.robot.update();
        TEST_ASSERT_EQUAL_INT(Lamp::Yellow, r.lights.lamp);
        r.clock.advance(1 + delta); r.robot.update();
        TEST_ASSERT_EQUAL_INT(RobotState::Red, r.robot.state());
        TEST_ASSERT_EQUAL(1, r.audio.stops);
        TEST_ASSERT_FALSE(r.lights.dancing);
        r.green(); r.sensor.event = true; r.robot.update();
        TEST_ASSERT_EQUAL(2, r.audio.plays);
    }
}
void audio_failure_keeps_sequence_responsive() {
    Rig r; r.audio.ok = false; r.reward();
    TEST_ASSERT_TRUE(r.log.contains("unavailable"));
    r.clock.advance(Config::RewardMs); r.robot.update();
    TEST_ASSERT_EQUAL_INT(RobotState::Red, r.robot.state());
    TEST_ASSERT_TRUE(r.log.contains("audio stop unavailable"));
    TEST_ASSERT_GREATER_THAN(0, r.audio.updates);
    TEST_ASSERT_GREATER_THAN(0, r.lights.updates);
}
void reset_cancels_reward_and_pending_event() {
    Rig r; r.reward(); r.sensor.event = true; r.robot.simulateHighFive();
    r.robot.begin(); r.green(); r.robot.update();
    TEST_ASSERT_EQUAL_INT(RobotState::GreenWaiting, r.robot.state());
    TEST_ASSERT_EQUAL(1, r.audio.stops);
    TEST_ASSERT_EQUAL(1, r.audio.plays);
}
void custom_settings_and_rollover() {
    Rig r;
    r.clock.time = UINT32_MAX - 3;
    RobotController robot(r.clock, r.sensor, r.audio, r.lights, r.log, {5, 6, 7, 42, 1, 4});
    robot.begin(); r.clock.advance(4); robot.update();
    TEST_ASSERT_EQUAL_INT(RobotState::Red, robot.state());
    r.clock.advance(1); robot.update();
    TEST_ASSERT_EQUAL_INT(RobotState::Yellow, robot.state());
    r.clock.advance(6); robot.update(); robot.simulateHighFive(); robot.update();
    TEST_ASSERT_EQUAL(42, r.audio.track);
    r.clock.advance(4); robot.update(); TEST_ASSERT_EQUAL_INT(Lamp::Yellow, r.lights.lamp);
    r.clock.advance(2); robot.update(); TEST_ASSERT_EQUAL_INT(RobotState::Reward, robot.state());
    r.clock.advance(1); robot.update();
    TEST_ASSERT_EQUAL_INT(RobotState::Red, robot.state());
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", RobotController::stateName(static_cast<RobotState>(99)));
}
void reward_tracks_are_randomized_without_immediate_repeats() {
    Rig r;
    for (int i = 0; i < 18; ++i) {
        r.green();
        r.clock.advance(static_cast<uint32_t>(i * 17 + 3));
        r.sensor.event = true; r.robot.update();
        r.clock.advance(Config::RewardMs); r.robot.update();
    }
    unsigned seen = 0;
    for (std::size_t i = 0; i < r.audio.tracks.size(); ++i) {
        const auto track = r.audio.tracks[i];
        TEST_ASSERT_GREATER_OR_EQUAL(Config::RewardTrack, track);
        TEST_ASSERT_LESS_THAN(Config::RewardTrack + Config::RewardTrackCount, track);
        seen |= 1u << (track - Config::RewardTrack);
        if (i) TEST_ASSERT_NOT_EQUAL(r.audio.tracks[i - 1], track);
    }
    TEST_ASSERT_EQUAL((1u << Config::RewardTrackCount) - 1, seen);
}
void physical_held_switch_requires_release_and_new_press() {
    FakeClock clock; FakeInput input; FakeAudio audio; FakeLights lights; FakeLog log;
    HighFiveSensor sensor(clock, input);
    sensor.begin();
    RobotController robot(clock, sensor, audio, lights, log, {1, 1, 1, 1});
    robot.begin(); clock.advance(1); robot.update(); clock.advance(1); robot.update();
    input.level = false; robot.update(); clock.advance(Config::DebounceMs); robot.update();
    TEST_ASSERT_EQUAL(1, audio.plays);
    for (int i = 0; i < 10; ++i) { clock.advance(1); robot.update(); }
    TEST_ASSERT_EQUAL_INT(RobotState::GreenWaiting, robot.state());
    TEST_ASSERT_EQUAL(1, audio.plays);
    input.level = true; robot.update(); clock.advance(Config::DebounceMs); robot.update();
    input.level = false; robot.update(); clock.advance(Config::DebounceMs); robot.update();
    TEST_ASSERT_EQUAL(2, audio.plays);
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(initial_red_and_inactive_update);
    RUN_TEST(red_and_yellow_boundaries);
    RUN_TEST(green_waits_indefinitely);
    RUN_TEST(early_events_are_consumed_not_deferred);
    RUN_TEST(reward_once_then_red_at_boundary);
    RUN_TEST(audio_failure_keeps_sequence_responsive);
    RUN_TEST(reset_cancels_reward_and_pending_event);
    RUN_TEST(custom_settings_and_rollover);
    RUN_TEST(physical_held_switch_requires_release_and_new_press);
    RUN_TEST(reward_tracks_are_randomized_without_immediate_repeats);
    return UNITY_END();
}

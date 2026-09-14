#include <unity.h>
#include "core/TrafficLight.h"
#include "../support/Fakes.h"
void setUp() {}
void tearDown() {}
void assertColor(Color expected, Color actual) {
    TEST_ASSERT_EQUAL(expected.r, actual.r); TEST_ASSERT_EQUAL(expected.g, actual.g); TEST_ASSERT_EQUAL(expected.b, actual.b);
}
void configured_groups_and_off() {
    FakeClock clock; FakePixels pixels(13);
    TrafficLight lights(clock, pixels, {{{0, 4}, {4, 4}, {8, 4}}});
    TEST_ASSERT_TRUE(lights.begin());
    const Color colors[] = {{255, 0, 0}, {255, 160, 0}, {0, 255, 0}};
    for (int lamp = 0; lamp < 3; ++lamp) {
        lights.show(static_cast<Lamp>(lamp));
        for (int i = 0; i < 13; ++i) assertColor(i / 4 == lamp ? colors[lamp] : Color{0, 0, 0}, pixels.pixels[i]);
    }
    lights.show(Lamp::Off);
    for (auto c : pixels.pixels) assertColor({0, 0, 0}, c);
}
void animation_boundary_progression_and_stop() {
    FakeClock clock; FakePixels pixels(6); TrafficLight lights(clock, pixels);
    lights.begin(); lights.updateAnimation(); TEST_ASSERT_EQUAL(1, pixels.shows);
    lights.startAnimation(); TEST_ASSERT_EQUAL(2, pixels.shows);
    for (int i = 0; i < 6; ++i) {
        const auto c = pixels.pixels[i]; TEST_ASSERT_TRUE(c.r || c.g || c.b);
    }
    assertColor({255, 0, 0}, pixels.pixels[0]);
    clock.advance(Config::AnimationMs - 1); lights.updateAnimation(); TEST_ASSERT_EQUAL(2, pixels.shows);
    clock.advance(1); lights.updateAnimation(); assertColor({0, 255, 0}, pixels.pixels[0]);
    clock.advance(Config::AnimationMs + 1); lights.updateAnimation(); assertColor({0, 0, 255}, pixels.pixels[0]);
    clock.advance(Config::AnimationMs); lights.updateAnimation(); assertColor({255, 0, 0}, pixels.pixels[0]);
    lights.show(Lamp::Green); const int shown = pixels.shows;
    clock.advance(1000); lights.updateAnimation(); TEST_ASSERT_EQUAL(shown, pixels.shows);
}
void animation_rollover_and_bounded_catchup() {
    FakeClock clock; FakePixels pixels; TrafficLight lights(clock, pixels);
    lights.begin(); clock.time = UINT32_MAX - 20; lights.startAnimation();
    clock.advance(Config::AnimationMs); lights.updateAnimation(); assertColor({0, 255, 0}, pixels.pixels[0]);
    const int shown = pixels.shows;
    clock.advance(Config::AnimationMs * 1000000); lights.updateAnimation();
    TEST_ASSERT_EQUAL(shown + 1, pixels.shows);
}
void invalid_layout_and_adapter_failures() {
    FakeClock clock; FakePixels pixels;
    const std::array<Config::Group, 3> invalid[] = {
        {{{0, 0}, {1, 1}, {2, 1}}}, {{{3, 1}, {1, 1}, {2, 1}}},
        {{{0, 4}, {1, 1}, {2, 1}}}, {{{0, 2}, {1, 1}, {2, 1}}},
        {{{0, 1}, {0, 1}, {2, 1}}}
    };
    for (const auto& groups : invalid) {
        TrafficLight lights(clock, pixels, groups); TEST_ASSERT_FALSE(lights.begin());
        lights.show(Lamp::Red); lights.startAnimation(); lights.updateAnimation();
    }
    TEST_ASSERT_EQUAL(0, pixels.begins); TEST_ASSERT_EQUAL(0, pixels.shows);
    TrafficLight zero(clock, pixels, Config::Lamps, 0); TEST_ASSERT_FALSE(zero.begin());
    pixels.ok = false; TrafficLight failed(clock, pixels); TEST_ASSERT_FALSE(failed.begin());
    TEST_ASSERT_EQUAL(0, pixels.shows);
}
int main() {
    UNITY_BEGIN(); RUN_TEST(configured_groups_and_off); RUN_TEST(animation_boundary_progression_and_stop);
    RUN_TEST(animation_rollover_and_bounded_catchup); RUN_TEST(invalid_layout_and_adapter_failures);
    return UNITY_END();
}

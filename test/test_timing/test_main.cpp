#include <unity.h>
#include "core/Timing.h"
#include <initializer_list>
void setUp() {}
void tearDown() {}
void unsigned_boundaries_and_wrap() {
    for (uint32_t start : {0u, 100u, UINT32_MAX - 15}) {
        for (uint32_t duration : {1u, 30u, 1000u, 3000u, 10000u, 30000u}) {
            TEST_ASSERT_FALSE(elapsed(start + duration - 1, start, duration));
            TEST_ASSERT_TRUE(elapsed(start + duration, start, duration));
            TEST_ASSERT_TRUE(elapsed(start + duration + 1, start, duration));
        }
    }
    TEST_ASSERT_TRUE(elapsed(42, 42, 0));
}
int main() { UNITY_BEGIN(); RUN_TEST(unsigned_boundaries_and_wrap); return UNITY_END(); }

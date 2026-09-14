#pragma once
#include <cstdint>

constexpr bool elapsed(uint32_t now, uint32_t start, uint32_t duration) {
    return static_cast<uint32_t>(now - start) >= duration;
}

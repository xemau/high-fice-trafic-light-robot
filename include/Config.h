#pragma once
#include "AppMode.h"
#include <array>
#include <cstddef>
#include <cstdint>

#ifndef DEFAULT_APP_MODE
#define DEFAULT_APP_MODE 1
#endif

namespace Config {
namespace Pins {
constexpr uint8_t Mp3Rx = 16; // ESP32 RX <- YX5200 TX
constexpr uint8_t Mp3Tx = 17; // ESP32 TX -> YX5200 RX
constexpr uint8_t LedData = 18;
constexpr uint8_t HighFive = 27;
}
constexpr uint32_t SerialBaud = 115200;
constexpr uint32_t Mp3Baud = 9600;
constexpr uint8_t Mp3Uart = 2;
constexpr std::size_t LedCount = 3;
struct Group { std::size_t first; std::size_t count; };
constexpr std::array<Group, 3> Lamps{{{0, 1}, {1, 1}, {2, 1}}};
constexpr uint8_t Brightness = 48;
enum class ColorOrder { GRB, RGB, BRG, BGR, RBG, GBR };
constexpr ColorOrder LedColorOrder = ColorOrder::GRB;
constexpr uint32_t RedMs = 3000;
constexpr uint32_t YellowMs = 1000;
constexpr uint32_t RewardMs = 10000;
constexpr uint32_t DebounceMs = 30;
constexpr uint32_t AnimationMs = 80;
constexpr uint32_t LightCycleMs = 1000;
constexpr uint32_t SelectionMs = 5000;
constexpr int DefaultVolume = 12;
constexpr uint16_t RewardTrack = 1;
constexpr int MaxVolume = 30;
constexpr uint16_t MaxTrack = 9999;
constexpr uint32_t AudioBootMs = 3000;
constexpr uint32_t AudioCommandMs = 200;
constexpr uint32_t AudioResponseMs = 1500;
constexpr uint32_t AudioPollMs = 5000;
constexpr uint32_t FrameTimeoutMs = 100;
constexpr std::size_t AudioQueueSize = 8;
constexpr std::size_t SerialLineSize = 64;
constexpr std::size_t IoBudget = 64;
constexpr std::size_t LogBufferSize = 2048;
constexpr auto DefaultMode = static_cast<AppMode>(DEFAULT_APP_MODE);
static_assert(DEFAULT_APP_MODE >= 1 && DEFAULT_APP_MODE <= 5, "DEFAULT_APP_MODE must be 1..5");
static_assert(DefaultVolume >= 0 && DefaultVolume <= MaxVolume);
static_assert(RewardTrack >= 1 && RewardTrack <= MaxTrack);
static_assert(AnimationMs > 0 && LightCycleMs > 0);
static_assert(LedCount > 0 && LedCount <= 65535 / 3, "RGB pixel buffer must fit the driver's 16-bit byte count");
constexpr bool validLayout() {
    for (std::size_t i = 0; i < Lamps.size(); ++i) {
        const auto a = Lamps[i];
        if (!a.count || a.first >= LedCount || a.count > LedCount - a.first) return false;
        for (std::size_t j = 0; j < i; ++j) {
            const auto b = Lamps[j];
            if (a.first < b.first + b.count && b.first < a.first + a.count) return false;
        }
    }
    return true;
}
static_assert(validLayout(), "Lamp groups must be nonempty, disjoint, and within LedCount");
}

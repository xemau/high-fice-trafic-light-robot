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
// Rows: top, middle, bottom pairs. Columns: red, green, blue cathodes.
constexpr std::array<std::array<uint8_t, 3>, 3> LedRgb{{
    {{18, 19, 23}}, {{25, 26, 32}}, {{33, 21, 22}}
}};
constexpr uint8_t HighFive = 27;
}
constexpr uint32_t SerialBaud = 115200;
constexpr uint32_t Mp3Baud = 9600;
constexpr uint8_t Mp3Uart = 2;
constexpr uint32_t RedMs = 3000;
constexpr uint32_t YellowMs = 1000;
constexpr uint32_t RewardMs = 30000;
constexpr uint32_t RewardWarningMs = 26000;
constexpr uint32_t DebounceMs = 30;
constexpr uint32_t AnimationFrameMs = 20;
constexpr uint32_t AnimationMinFadeMs = 2000;
constexpr uint32_t AnimationMaxFadeMs = 3500;
constexpr uint32_t LedPwmHz = 5000;
constexpr uint8_t LedPwmBits = 8;
constexpr uint32_t LightCycleMs = 1000;
constexpr uint32_t SelectionMs = 5000;
constexpr int DefaultVolume = 30;
constexpr uint8_t DefaultEqualizer = 3; // YX5200: Normal=0, Pop=1, Rock=2, Jazz=3, Classic=4, Bass=5.
constexpr uint16_t RewardTrack = 1;
constexpr uint16_t RewardTrackCount = 596;
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
static_assert(DefaultEqualizer <= 5);
static_assert(RewardWarningMs < RewardMs);
static_assert(RewardTrack >= 1 && RewardTrack <= MaxTrack);
static_assert(RewardTrackCount >= 1 && RewardTrack + RewardTrackCount - 1 <= MaxTrack);
static_assert(AnimationFrameMs > 0 && AnimationFrameMs <= AnimationMinFadeMs);
static_assert(AnimationMinFadeMs <= AnimationMaxFadeMs);
static_assert(LedPwmHz >= 1000 && LedPwmBits == 8);
static_assert(Pins::LedRgb.size() * 3 <= 16, "ESP32 has 16 LEDC channels");
static_assert(LightCycleMs > 0);
constexpr bool validLedPins() {
    constexpr std::array<uint8_t, 15> outputs{{4, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33}};
    uint64_t used = (1ULL << Pins::Mp3Rx) | (1ULL << Pins::Mp3Tx) | (1ULL << Pins::HighFive);
    for (const auto& pair : Pins::LedRgb) for (const auto pin : pair) {
        bool valid = false;
        for (const auto output : outputs) if (pin == output) valid = true;
        if (!valid || (used & (1ULL << pin))) return false;
        used |= 1ULL << pin;
    }
    return true;
}
static_assert(validLedPins(), "RGB channels need distinct non-strapping output GPIOs, separate from audio/sensor");
}

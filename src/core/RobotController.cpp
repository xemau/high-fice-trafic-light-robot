#include "core/RobotController.h"
#include "core/Timing.h"
#include <cstdio>

uint16_t RobotController::selectRewardTrack() {
    if (settings_.rewardTrackCount <= 1) return settings_.rewardTrack;
    randomState_ ^= clock_.now() + 0x9e3779b9 + (randomState_ << 6) + (randomState_ >> 2);
    randomState_ ^= randomState_ << 13;
    randomState_ ^= randomState_ >> 17;
    randomState_ ^= randomState_ << 5;
    uint16_t offset = static_cast<uint16_t>(randomState_ % settings_.rewardTrackCount);
    uint16_t track = static_cast<uint16_t>(settings_.rewardTrack + offset);
    if (track == lastRewardTrack_) {
        offset = static_cast<uint16_t>((offset + 1 +
            (randomState_ / settings_.rewardTrackCount) % (settings_.rewardTrackCount - 1)) %
            settings_.rewardTrackCount);
        track = static_cast<uint16_t>(settings_.rewardTrack + offset);
    }
    lastRewardTrack_ = track;
    return track;
}

const char* RobotController::stateName(RobotState state) {
    switch (state) {
        case RobotState::Red: return "RED";
        case RobotState::Yellow: return "YELLOW";
        case RobotState::GreenWaiting: return "GREEN";
        case RobotState::Reward: return "REWARD";
    }
    return "UNKNOWN";
}

void RobotController::begin() {
    if (active_) audio_.stop();
    sensor_.takePress();
    simulated_ = false;
    active_ = true;
    enter(RobotState::Red);
}

void RobotController::enter(RobotState state) {
    state_ = state;
    startedAt_ = clock_.now();
    switch (state) {
        case RobotState::Red: lights_.show(Lamp::Red); break;
        case RobotState::Yellow: lights_.show(Lamp::Yellow); break;
        case RobotState::GreenWaiting: lights_.show(Lamp::Green); break;
        case RobotState::Reward: {
            rewardWarningShown_ = false;
            const uint16_t track = selectRewardTrack();
            const bool ok = audio_.playTrack(track);
            char message[64];
            std::snprintf(message, sizeof(message), "[AUDIO] PLAY %u %s", track,
                          ok ? "queued" : "unavailable");
            log_.log(message);
            lights_.startAnimation();
            break;
        }
    }
    char message[32];
    std::snprintf(message, sizeof(message), "[STATE] %s", stateName(state));
    log_.log(message);
}

void RobotController::update() {
    if (!active_) return;
    sensor_.update();
    const bool physical = sensor_.takePress();
    const bool press = physical || simulated_;
    simulated_ = false;
    if (press) log_.log("[SENSOR] HIGH FIVE");
    const auto now = clock_.now();
    switch (state_) {
        case RobotState::Red:
            if (elapsed(now, startedAt_, settings_.redMs)) enter(RobotState::Yellow);
            break;
        case RobotState::Yellow:
            if (elapsed(now, startedAt_, settings_.yellowMs)) enter(RobotState::GreenWaiting);
            break;
        case RobotState::GreenWaiting:
            if (press) enter(RobotState::Reward);
            break;
        case RobotState::Reward:
            if (elapsed(now, startedAt_, settings_.rewardMs)) {
                if (!audio_.stop()) log_.log("[ERROR] audio stop unavailable");
                enter(RobotState::Red);
            } else if (!rewardWarningShown_ && elapsed(now, startedAt_, settings_.rewardWarningMs)) {
                rewardWarningShown_ = true;
                lights_.show(Lamp::YellowGreen);
            }
            break;
    }
    audio_.update();
    lights_.updateAnimation();
}

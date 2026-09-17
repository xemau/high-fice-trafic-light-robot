#include "core/RobotController.h"
#include "core/Timing.h"
#include <cstdio>

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
    if (active_ && !audio_.stop()) log_.log("[ERROR] robot reset could not queue audio stop");
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
            const bool ok = audio_.playTrack(settings_.rewardTrack);
            char message[160];
            std::snprintf(message, sizeof(message), "[AUDIO] PLAY %u %s reason=high-five reward audio=%s limit_ms=%lu",
                          static_cast<unsigned>(settings_.rewardTrack), ok ? "queued" : "unavailable",
                          audioStatusName(audio_.status()), static_cast<unsigned long>(settings_.rewardMs));
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
                log_.log("[AUDIO] stop requested reason=reward time limit reached");
                if (!audio_.stop()) log_.log("[ERROR] audio stop unavailable");
                enter(RobotState::Red);
            }
            break;
    }
    audio_.update();
    lights_.updateAnimation();
}

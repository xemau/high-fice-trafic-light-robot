#pragma once
#include "Config.h"
#include "interfaces/Interfaces.h"

enum class RobotState { Red, Yellow, GreenWaiting, Reward };
struct RobotSettings {
    uint32_t redMs = Config::RedMs, yellowMs = Config::YellowMs, rewardMs = Config::RewardMs;
    uint16_t rewardTrack = Config::RewardTrack;
    uint16_t rewardTrackCount = Config::RewardTrackCount;
    uint32_t rewardWarningMs = Config::RewardWarningMs;
};
class RobotController {
public:
    RobotController(IClock& clock, IHighFiveSensor& sensor, IAudioPlayer& audio,
                    ITrafficLight& lights, ILogger& log, RobotSettings settings = {})
        : clock_(clock), sensor_(sensor), audio_(audio), lights_(lights), log_(log), settings_(settings) {}
    void begin();
    void update();
    void seedRandom(uint32_t seed) { randomState_ = seed ? seed : 0x9e3779b9; }
    void simulateHighFive() { simulated_ = true; }
    RobotState state() const { return state_; }
    static const char* stateName(RobotState state);
private:
    void enter(RobotState state);
    uint16_t selectRewardTrack();
    IClock& clock_;
    IHighFiveSensor& sensor_;
    IAudioPlayer& audio_;
    ITrafficLight& lights_;
    ILogger& log_;
    RobotSettings settings_;
    RobotState state_ = RobotState::Red;
    uint32_t startedAt_ = 0, randomState_ = 0x9e3779b9;
    uint16_t lastRewardTrack_ = 0;
    bool active_ = false, simulated_ = false, rewardWarningShown_ = false;
};

#pragma once
#include "Config.h"
#include "interfaces/Interfaces.h"

class TrafficLight final : public ITrafficLight {
public:
    TrafficLight(IClock& clock, ILedOutputs& outputs,
                 uint32_t interval = Config::AnimationFrameMs)
        : clock_(clock), outputs_(outputs), interval_(interval) {}
    bool begin() override;
    void show(Lamp lamp) override;
    void startAnimation() override;
    void updateAnimation() override;
    void seedAnimation(uint32_t seed);
private:
    static RgbChannels hsv(uint8_t hue, uint8_t saturation, uint8_t value);
    static uint8_t interpolate(uint8_t from, uint8_t to, uint32_t elapsed, uint32_t duration);
    uint32_t nextRandom();
    void beginTransition(uint32_t now);
    void renderTransition(uint32_t elapsed);
    IClock& clock_;
    ILedOutputs& outputs_;
    LedFrame currentFrame_{}, fromFrame_{}, targetFrame_{};
    uint32_t interval_, lastFrame_ = 0, transitionStarted_ = 0, transitionMs_ = 0;
    uint32_t randomState_ = 0x6d2b79f5;
    bool active_ = false, animating_ = false;
};

#pragma once
#include "Config.h"
#include "interfaces/Interfaces.h"

class TrafficLight final : public ITrafficLight {
public:
    TrafficLight(IClock& clock, ILedOutputs& outputs,
                 uint32_t interval = Config::AnimationMs)
        : clock_(clock), outputs_(outputs), interval_(interval) {}
    bool begin() override;
    void show(Lamp lamp) override;
    void startAnimation() override;
    void updateAnimation() override;
private:
    void renderAnimation();
    IClock& clock_;
    ILedOutputs& outputs_;
    uint32_t interval_, lastFrame_ = 0;
    uint8_t phase_ = 0;
    bool active_ = false, animating_ = false;
};

#pragma once
#include "Config.h"
#include "interfaces/Interfaces.h"

class TrafficLight final : public ITrafficLight {
public:
    TrafficLight(IClock& clock, IPixels& pixels,
                 std::array<Config::Group, 3> groups = Config::Lamps,
                 uint32_t interval = Config::AnimationMs)
        : clock_(clock), pixels_(pixels), groups_(groups), interval_(interval) {}
    bool begin() override;
    void show(Lamp lamp) override;
    void startAnimation() override;
    void updateAnimation() override;
private:
    void renderAnimation();
    IClock& clock_;
    IPixels& pixels_;
    std::array<Config::Group, 3> groups_;
    uint32_t interval_, lastFrame_ = 0;
    uint8_t phase_ = 0;
    bool active_ = false, animating_ = false;
};

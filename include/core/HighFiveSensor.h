#pragma once
#include "Config.h"
#include "interfaces/Interfaces.h"

class HighFiveSensor final : public IHighFiveSensor {
public:
    HighFiveSensor(IClock& clock, IDigitalInput& input, uint32_t debounce = Config::DebounceMs)
        : clock_(clock), input_(input), debounce_(debounce) {}
    bool begin() override;
    void update() override;
    bool pressed() const override { return stable_; }
    bool takePress() override;
private:
    IClock& clock_;
    IDigitalInput& input_;
    uint32_t debounce_, changedAt_ = 0;
    bool active_ = false, raw_ = false, stable_ = false, event_ = false;
};

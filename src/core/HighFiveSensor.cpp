#include "core/HighFiveSensor.h"
#include "core/Timing.h"

bool HighFiveSensor::begin() {
    active_ = input_.begin();
    raw_ = stable_ = active_ && !input_.high();
    event_ = false;
    changedAt_ = clock_.now();
    return active_;
}

void HighFiveSensor::update() {
    if (!active_) return;
    const auto now = clock_.now();
    const bool pressed = !input_.high();
    if (pressed != raw_) {
        raw_ = pressed;
        changedAt_ = now;
    }
    if (raw_ != stable_ && elapsed(now, changedAt_, debounce_)) {
        stable_ = raw_;
        if (stable_) event_ = true;
    }
}

bool HighFiveSensor::takePress() {
    const bool result = event_;
    event_ = false;
    return result;
}

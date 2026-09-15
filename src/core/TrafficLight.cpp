#include "core/TrafficLight.h"
#include "core/Timing.h"

bool TrafficLight::begin() {
    active_ = false;
    animating_ = false;
    if (!interval_) return false;
    active_ = outputs_.begin();
    show(Lamp::Off);
    return active_;
}

void TrafficLight::show(Lamp lamp) {
    animating_ = false;
    if (!active_) return;
    outputs_.write(lamp == Lamp::Red, lamp == Lamp::Yellow, lamp == Lamp::Green);
}

void TrafficLight::startAnimation() {
    if (!active_) return;
    animating_ = true;
    phase_ = 0;
    lastFrame_ = clock_.now();
    renderAnimation();
}

void TrafficLight::updateAnimation() {
    if (!animating_) return;
    const auto now = clock_.now();
    if (!elapsed(now, lastFrame_, interval_)) return;
    const uint32_t steps = static_cast<uint32_t>(now - lastFrame_) / interval_;
    lastFrame_ += steps * interval_;
    phase_ = static_cast<uint8_t>((phase_ + steps % 3) % 3);
    renderAnimation();
}

void TrafficLight::renderAnimation() {
    outputs_.write(phase_ == 0, phase_ == 1, phase_ == 2);
}

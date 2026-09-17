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
    LedFrame frame{};
    if (lamp == Lamp::Red) frame[0] = {true, false, false};
    if (lamp == Lamp::Yellow || lamp == Lamp::YellowGreen) frame[1] = {true, true, false};
    if (lamp == Lamp::Green || lamp == Lamp::YellowGreen) frame[2] = {false, true, false};
    outputs_.write(frame);
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
    constexpr RgbChannels colors[] = {{true, false, false}, {false, true, false}, {false, false, true}};
    LedFrame frame{};
    for (std::size_t i = 0; i < frame.size(); ++i) frame[i] = colors[(i + phase_) % 3];
    outputs_.write(frame);
}

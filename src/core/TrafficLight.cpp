#include "core/TrafficLight.h"
#include "core/Timing.h"

bool TrafficLight::begin() {
    active_ = false;
    if (!interval_) return false;
    for (std::size_t i = 0; i < groups_.size(); ++i) {
        const auto a = groups_[i];
        if (!a.count || a.first >= pixels_.size() || a.count > pixels_.size() - a.first) return false;
        for (std::size_t j = 0; j < i; ++j) {
            const auto b = groups_[j];
            if (a.first < b.first + b.count && b.first < a.first + a.count) return false;
        }
    }
    active_ = pixels_.begin();
    show(Lamp::Off);
    return active_;
}

void TrafficLight::show(Lamp lamp) {
    animating_ = false;
    if (!active_) return;
    for (std::size_t i = 0; i < pixels_.size(); ++i) pixels_.set(i, {0, 0, 0});
    const auto index = static_cast<std::size_t>(lamp);
    if (index < groups_.size()) {
        constexpr Color colors[] = {{255, 0, 0}, {255, 160, 0}, {0, 255, 0}};
        const auto group = groups_[index];
        for (std::size_t i = group.first; i < group.first + group.count; ++i) pixels_.set(i, colors[index]);
    }
    pixels_.show();
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
    constexpr Color colors[] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}};
    for (std::size_t i = 0; i < pixels_.size(); ++i) pixels_.set(i, colors[(i + phase_) % 3]);
    pixels_.show();
}

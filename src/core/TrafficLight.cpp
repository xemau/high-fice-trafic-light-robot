#include "core/TrafficLight.h"
#include "core/Timing.h"

namespace {
constexpr uint8_t Full = 255;
constexpr uint8_t MinimumSaturation = 176;
constexpr uint8_t MinimumValue = 128;
constexpr uint8_t ValueRange = 96;
}

bool TrafficLight::begin() {
    active_ = false;
    animating_ = false;
    if (!interval_ || interval_ > Config::AnimationMinFadeMs) return false;
    active_ = outputs_.begin();
    show(Lamp::Off);
    return active_;
}

void TrafficLight::show(Lamp lamp) {
    animating_ = false;
    if (!active_) return;
    LedFrame frame{};
    if (lamp == Lamp::Red) frame[0] = {Full, 0, 0};
    if (lamp == Lamp::Yellow) frame[1] = {Full, Full, 0};
    if (lamp == Lamp::Green) frame[2] = {0, Full, 0};
    currentFrame_ = frame;
    outputs_.write(currentFrame_);
}

void TrafficLight::seedAnimation(uint32_t seed) {
    randomState_ = seed ? seed : 0x6d2b79f5;
}

uint32_t TrafficLight::nextRandom() {
    randomState_ ^= randomState_ << 13;
    randomState_ ^= randomState_ >> 17;
    randomState_ ^= randomState_ << 5;
    return randomState_;
}

RgbChannels TrafficLight::hsv(uint8_t hue, uint8_t saturation, uint8_t value) {
    const uint8_t region = hue / 43;
    const uint8_t remainder = static_cast<uint8_t>((hue - region * 43) * 6);
    const uint8_t p = static_cast<uint8_t>((value * (255 - saturation)) >> 8);
    const uint8_t q = static_cast<uint8_t>((value * (255 - ((saturation * remainder) >> 8))) >> 8);
    const uint8_t t = static_cast<uint8_t>((value * (255 - ((saturation * (255 - remainder)) >> 8))) >> 8);
    switch (region) {
        case 0: return {value, t, p};
        case 1: return {q, value, p};
        case 2: return {p, value, t};
        case 3: return {p, q, value};
        case 4: return {t, p, value};
        default: return {value, p, q};
    }
}

uint8_t TrafficLight::interpolate(uint8_t from, uint8_t to, uint32_t elapsed, uint32_t duration) {
    const uint32_t remaining = duration - elapsed;
    return static_cast<uint8_t>((static_cast<uint32_t>(from) * remaining +
                                 static_cast<uint32_t>(to) * elapsed + duration / 2) / duration);
}

void TrafficLight::beginTransition(uint32_t now) {
    fromFrame_ = currentFrame_;
    const uint8_t baseHue = static_cast<uint8_t>(nextRandom());
    const uint8_t hueSpacing = static_cast<uint8_t>(48 + nextRandom() % 65);
    const bool reverse = (nextRandom() & 1u) != 0;
    for (std::size_t i = 0; i < targetFrame_.size(); ++i) {
        const int direction = reverse ? -1 : 1;
        const int jitter = static_cast<int>(nextRandom() % 25) - 12;
        const uint8_t hue = static_cast<uint8_t>(baseHue + direction * static_cast<int>(i * hueSpacing) + jitter);
        const uint8_t saturation = static_cast<uint8_t>(MinimumSaturation + nextRandom() % (Full - MinimumSaturation + 1));
        const uint8_t value = static_cast<uint8_t>(MinimumValue + nextRandom() % ValueRange);
        targetFrame_[i] = hsv(hue, saturation, value);
    }
    transitionStarted_ = now;
    transitionMs_ = Config::AnimationMinFadeMs +
        nextRandom() % (Config::AnimationMaxFadeMs - Config::AnimationMinFadeMs + 1);
}

void TrafficLight::startAnimation() {
    if (!active_) return;
    animating_ = true;
    const auto now = clock_.now();
    randomState_ ^= now + 0x9e3779b9 + (randomState_ << 6) + (randomState_ >> 2);
    if (!randomState_) randomState_ = 0x6d2b79f5;
    lastFrame_ = now;
    beginTransition(now);
    outputs_.write(currentFrame_);
}

void TrafficLight::renderTransition(uint32_t elapsed) {
    for (std::size_t i = 0; i < currentFrame_.size(); ++i) {
        currentFrame_[i].red = interpolate(fromFrame_[i].red, targetFrame_[i].red, elapsed, transitionMs_);
        currentFrame_[i].green = interpolate(fromFrame_[i].green, targetFrame_[i].green, elapsed, transitionMs_);
        currentFrame_[i].blue = interpolate(fromFrame_[i].blue, targetFrame_[i].blue, elapsed, transitionMs_);
    }
    outputs_.write(currentFrame_);
}

void TrafficLight::updateAnimation() {
    if (!animating_) return;
    const auto now = clock_.now();
    if (!elapsed(now, lastFrame_, interval_)) return;
    lastFrame_ = now;
    const uint32_t transitionElapsed = now - transitionStarted_;
    if (transitionElapsed >= transitionMs_) {
        currentFrame_ = targetFrame_;
        outputs_.write(currentFrame_);
        beginTransition(now);
        return;
    }
    renderTransition(transitionElapsed);
}

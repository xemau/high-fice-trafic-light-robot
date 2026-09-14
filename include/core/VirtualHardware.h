#pragma once
#include "interfaces/Interfaces.h"
#include "Config.h"

class VirtualSensor final : public IHighFiveSensor {
public:
    bool begin() override { return true; }
    void update() override {}
    bool pressed() const override { return false; }
    bool takePress() override { return false; }
};
class VirtualLights final : public ITrafficLight {
public:
    bool begin() override { return true; }
    void show(Lamp) override {}
    void startAnimation() override {}
    void updateAnimation() override {}
};
class VirtualAudio final : public IAudioPlayer {
public:
    bool begin() override { active_ = true; return true; }
    void update() override {}
    bool playTrack(uint16_t track) override { return active_ && track >= 1 && track <= Config::MaxTrack; }
    bool stop() override { return active_; }
    bool pause() override { return active_; }
    bool resume() override { return active_; }
    bool setVolume(int volume) override { return active_ && volume >= 0 && volume <= Config::MaxVolume; }
    bool next() override { return active_; }
    bool previous() override { return active_; }
    AudioStatus status() const override { return active_ ? AudioStatus::Ready : AudioStatus::Off; }
private:
    bool active_ = false;
};

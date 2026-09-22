#pragma once
#include "interfaces/Interfaces.h"
#include "core/YX5200AudioPlayer.h"
#include <deque>
#include <string>
#include <vector>

struct FakeClock : IClock {
    uint32_t time = 0;
    uint32_t now() const override { return time; }
    void advance(uint32_t ms) { time += ms; }
};
struct FakeLog : ILogger {
    std::vector<std::string> messages;
    void log(const char* message) override { messages.emplace_back(message); }
    bool contains(const char* text) const {
        for (const auto& message : messages) if (message.find(text) != std::string::npos) return true;
        return false;
    }
};
struct FakeInput : IDigitalInput {
    bool level = true, ok = true;
    int begins = 0;
    bool begin() override { ++begins; return ok; }
    bool high() const override { return level; }
};
struct FakeSensor : IHighFiveSensor {
    bool down = false, event = false, ok = true;
    int begins = 0, updates = 0, takes = 0;
    bool begin() override { ++begins; return ok; }
    void update() override { ++updates; }
    bool pressed() const override { return down; }
    bool takePress() override { ++takes; const bool p = event; event = false; return p; }
};
struct FakeAudio : IAudioPlayer {
    bool ok = true;
    int begins = 0, updates = 0, plays = 0, stops = 0, pauses = 0, resumes = 0, nexts = 0, previouses = 0;
    uint16_t track = 0;
    std::vector<uint16_t> tracks;
    int volume = -1;
    AudioStatus state = AudioStatus::Off;
    bool begin() override { ++begins; state = ok ? AudioStatus::Ready : AudioStatus::Failed; return ok; }
    void update() override { ++updates; }
    bool playTrack(uint16_t value) override { ++plays; track = value; tracks.push_back(value); return ok; }
    bool stop() override { ++stops; return ok; }
    bool pause() override { ++pauses; return ok; }
    bool resume() override { ++resumes; return ok; }
    bool setVolume(int value) override { volume = value; return ok; }
    bool next() override { ++nexts; return ok; }
    bool previous() override { ++previouses; return ok; }
    AudioStatus status() const override { return state; }
};
struct FakeLights : ITrafficLight {
    bool ok = true, dancing = false;
    int begins = 0, animations = 0, updates = 0, shows = 0;
    Lamp lamp = Lamp::Off;
    bool begin() override { ++begins; return ok; }
    void show(Lamp value) override { ++shows; lamp = value; dancing = false; }
    void startAnimation() override { ++animations; dancing = true; }
    void updateAnimation() override { ++updates; }
};
struct FakeLedOutputs : ILedOutputs {
    bool ok = true;
    int begins = 0, writes = 0;
    LedFrame frame{};
    bool begin() override { ++begins; return ok; }
    void write(const LedFrame& value) override { ++writes; frame = value; }
};
struct FakeUart : IUart {
    bool ok = true, writeOk = true;
    int begins = 0, reads = 0;
    std::deque<uint8_t> rx;
    std::vector<Mp3Bytes> tx;
    bool begin() override { ++begins; return ok; }
    int read() override {
        ++reads;
        if (rx.empty()) return -1;
        const auto b = rx.front(); rx.pop_front(); return b;
    }
    bool write(const uint8_t* bytes, std::size_t size) override {
        if (!writeOk || size != 10) return false;
        Mp3Bytes frame{};
        for (std::size_t i = 0; i < size; ++i) frame[i] = bytes[i];
        tx.push_back(frame);
        return true;
    }
    void respond(uint8_t command, uint16_t parameter) {
        const auto frame = encodeMp3Frame(command, parameter);
        for (const auto byte : frame) rx.push_back(byte);
    }
};

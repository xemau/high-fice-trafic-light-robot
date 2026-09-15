#pragma once
#include <cstddef>
#include <cstdint>

struct IClock {
    virtual ~IClock() = default;
    virtual uint32_t now() const = 0;
};
struct ILogger {
    virtual ~ILogger() = default;
    virtual void log(const char* message) = 0;
};
struct IDigitalInput {
    virtual ~IDigitalInput() = default;
    virtual bool begin() = 0;
    virtual bool high() const = 0;
};
struct IHighFiveSensor {
    virtual ~IHighFiveSensor() = default;
    virtual bool begin() = 0;
    virtual void update() = 0;
    virtual bool pressed() const = 0;
    virtual bool takePress() = 0;
};
enum class AudioStatus { Off, Starting, Ready, Failed };
struct IAudioPlayer {
    virtual ~IAudioPlayer() = default;
    virtual bool begin() = 0;
    virtual void update() = 0;
    virtual bool playTrack(uint16_t track) = 0;
    virtual bool stop() = 0;
    virtual bool pause() = 0;
    virtual bool resume() = 0;
    virtual bool setVolume(int volume) = 0;
    virtual bool next() = 0;
    virtual bool previous() = 0;
    virtual AudioStatus status() const = 0;
    virtual bool takeError() { return false; }
};
enum class Lamp { Red, Yellow, Green, Off };
struct ITrafficLight {
    virtual ~ITrafficLight() = default;
    virtual bool begin() = 0;
    virtual void show(Lamp lamp) = 0;
    virtual void startAnimation() = 0;
    virtual void updateAnimation() = 0;
};
struct ILedOutputs {
    virtual ~ILedOutputs() = default;
    virtual bool begin() = 0;
    virtual void write(bool red, bool yellow, bool green) = 0;
};
struct IUart {
    virtual ~IUart() = default;
    virtual bool begin() = 0;
    virtual int read() = 0; // -1 when no byte is available
    virtual bool write(const uint8_t* bytes, std::size_t size) = 0;
};

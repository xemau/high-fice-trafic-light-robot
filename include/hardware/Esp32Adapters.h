#pragma once
#include "Config.h"
#include "interfaces/Interfaces.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class Esp32Clock final : public IClock {
public:
    uint32_t now() const override { return millis(); }
};
class Esp32Input final : public IDigitalInput {
public:
    bool begin() override;
    bool high() const override;
};
class Esp32Uart final : public IUart {
public:
    Esp32Uart() : serial_(Config::Mp3Uart) {}
    bool begin() override;
    int read() override;
    bool write(const uint8_t* bytes, std::size_t size) override;
private:
    HardwareSerial serial_;
    bool active_ = false;
};
class Ws2812Pixels final : public IPixels {
public:
    bool begin() override;
    std::size_t size() const override { return Config::LedCount; }
    void set(std::size_t index, Color color) override;
    void show() override;
private:
    Adafruit_NeoPixel strip_;
};
class SerialConsole final : public ILogger {
public:
    void begin();
    int read();
    void log(const char* message) override;
    void update();
private:
    char buffer_[Config::LogBufferSize]{};
    std::size_t head_ = 0, size_ = 0;
    bool dropped_ = false;
};

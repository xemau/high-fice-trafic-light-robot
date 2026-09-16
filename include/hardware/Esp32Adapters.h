#pragma once
#include "Config.h"
#include "interfaces/Interfaces.h"
#include <Arduino.h>

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
class Esp32LedOutputs final : public ILedOutputs {
public:
    bool begin() override;
    void write(const LedFrame& frame) override;
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

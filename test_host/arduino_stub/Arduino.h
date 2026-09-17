#pragma once
#include <cstddef>
#include <cstdint>

constexpr uint8_t LOW = 0, HIGH = 1, OUTPUT = 3, INPUT_PULLUP = 5;
constexpr uint32_t SERIAL_8N1 = 0;
uint32_t millis();
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
int digitalRead(uint8_t pin);
uint32_t ledcSetup(uint8_t channel, uint32_t frequency, uint8_t resolutionBits);
void ledcAttachPin(uint8_t pin, uint8_t channel);
void ledcWrite(uint8_t channel, uint32_t duty);
class HardwareSerial {
public:
    explicit HardwareSerial(uint8_t = 0) {}
    void begin(uint32_t, uint32_t = SERIAL_8N1, int = -1, int = -1) {}
    void end() {}
    explicit operator bool() const { return true; }
    int read() { return -1; }
    int availableForWrite() { return 64; }
    std::size_t write(const uint8_t*, std::size_t size) { return size; }
    std::size_t write(uint8_t) { return 1; }
};
extern HardwareSerial Serial;

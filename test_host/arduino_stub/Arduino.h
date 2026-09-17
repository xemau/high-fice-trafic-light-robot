#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

constexpr uint8_t LOW = 0, HIGH = 1, OUTPUT = 3, INPUT_PULLUP = 5;
constexpr uint32_t SERIAL_8N1 = 0;
uint32_t millis();
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
int digitalRead(uint8_t pin);
class HardwareSerial {
public:
    std::string output;
    int writeSpace = 64;
    explicit HardwareSerial(uint8_t = 0) {}
    void begin(uint32_t, uint32_t = SERIAL_8N1, int = -1, int = -1) {}
    void end() {}
    explicit operator bool() const { return true; }
    int read() { return -1; }
    int availableForWrite() { return writeSpace; }
    std::size_t write(const uint8_t*, std::size_t size) { return size; }
    std::size_t write(uint8_t byte) { output.push_back(static_cast<char>(byte)); return 1; }
};
extern HardwareSerial Serial;

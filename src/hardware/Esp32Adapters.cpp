#include "hardware/Esp32Adapters.h"
#include <cstring>
#include <cstdio>

bool Esp32Input::begin() {
    pinMode(Config::Pins::HighFive, INPUT_PULLUP);
    return true;
}
bool Esp32Input::high() const { return digitalRead(Config::Pins::HighFive) == HIGH; }

bool Esp32Uart::begin() {
    if (active_) serial_.end();
    serial_.begin(Config::Mp3Baud, SERIAL_8N1, Config::Pins::Mp3Rx, Config::Pins::Mp3Tx);
    active_ = static_cast<bool>(serial_);
    return active_;
}
int Esp32Uart::read() { return serial_.read(); }
bool Esp32Uart::write(const uint8_t* bytes, std::size_t size) {
    if (serial_.availableForWrite() < static_cast<int>(size)) return false;
    return serial_.write(bytes, size) == size;
}

bool Esp32LedOutputs::begin() {
    for (const auto& pair : Config::Pins::LedRgb) for (const auto pin : pair) {
        digitalWrite(pin, HIGH);
        pinMode(pin, OUTPUT);
    }
    return true;
}
void Esp32LedOutputs::write(const LedFrame& frame) {
    // Common-anode channels are active low; blank the old frame first.
    for (const auto& pair : Config::Pins::LedRgb) for (const auto pin : pair) digitalWrite(pin, HIGH);
    for (std::size_t i = 0; i < frame.size(); ++i) {
        if (frame[i].red) digitalWrite(Config::Pins::LedRgb[i][0], LOW);
        if (frame[i].green) digitalWrite(Config::Pins::LedRgb[i][1], LOW);
        if (frame[i].blue) digitalWrite(Config::Pins::LedRgb[i][2], LOW);
    }
}

void SerialConsole::begin() { Serial.begin(Config::SerialBaud); }
int SerialConsole::read() { return Serial.read(); }
void SerialConsole::log(const char* message) {
    const auto length = std::strlen(message);
    char timestamp[24];
    const auto prefix = static_cast<std::size_t>(std::snprintf(timestamp, sizeof(timestamp), "[t=%lu] ",
                                                             static_cast<unsigned long>(millis())));
    if (prefix + length + 1 > sizeof(buffer_) - size_) {
        dropped_ = true;
        return;
    }
    for (std::size_t i = 0; i < prefix; ++i) buffer_[(head_ + size_++) % sizeof(buffer_)] = timestamp[i];
    for (std::size_t i = 0; i < length; ++i) buffer_[(head_ + size_++) % sizeof(buffer_)] = message[i];
    buffer_[(head_ + size_++) % sizeof(buffer_)] = '\n';
}
void SerialConsole::update() {
    for (std::size_t i = 0; i < Config::IoBudget && size_ && Serial.availableForWrite() > 0; ++i) {
        Serial.write(static_cast<uint8_t>(buffer_[head_]));
        head_ = (head_ + 1) % sizeof(buffer_);
        --size_;
    }
    if (dropped_ && size_ == 0) {
        dropped_ = false;
        log("[WARN] serial log overflow; messages dropped");
    }
}

#include "hardware/Esp32Adapters.h"
#include <cstring>

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
    uint8_t channel = 0;
    for (const auto& pair : Config::Pins::LedRgb) for (const auto pin : pair) {
        digitalWrite(pin, HIGH);
        pinMode(pin, OUTPUT);
        if (!ledcSetup(channel, Config::LedPwmHz, Config::LedPwmBits)) return false;
        ledcAttachPin(pin, channel);
        ledcWrite(channel, 255);
        ++channel;
    }
    return true;
}
void Esp32LedOutputs::write(const LedFrame& frame) {
    uint8_t channel = 0;
    for (std::size_t i = 0; i < frame.size(); ++i) {
        const uint8_t brightness[] = {frame[i].red, frame[i].green, frame[i].blue};
        for (const auto value : brightness) {
            const uint32_t gammaCorrected = (static_cast<uint32_t>(value) * value + 127) / 255;
            ledcWrite(channel++, 255 - gammaCorrected);
        }
    }
}

void SerialConsole::begin() { Serial.begin(Config::SerialBaud); }
int SerialConsole::read() { return Serial.read(); }
void SerialConsole::log(const char* message) {
    const auto length = std::strlen(message);
    if (length + 1 > sizeof(buffer_) - size_) {
        dropped_ = true;
        return;
    }
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

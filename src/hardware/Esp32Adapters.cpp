#include "hardware/Esp32Adapters.h"
#include <cstring>
#include <initializer_list>

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
    for (const auto pin : {Config::Pins::LedRed, Config::Pins::LedYellow, Config::Pins::LedGreen}) {
        digitalWrite(pin, LOW);
        pinMode(pin, OUTPUT);
    }
    return true;
}
void Esp32LedOutputs::write(bool red, bool yellow, bool green) {
    // Turn off the previous lamp before enabling the next one.
    digitalWrite(Config::Pins::LedRed, LOW);
    digitalWrite(Config::Pins::LedYellow, LOW);
    digitalWrite(Config::Pins::LedGreen, LOW);
    if (red) digitalWrite(Config::Pins::LedRed, HIGH);
    if (yellow) digitalWrite(Config::Pins::LedYellow, HIGH);
    if (green) digitalWrite(Config::Pins::LedGreen, HIGH);
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

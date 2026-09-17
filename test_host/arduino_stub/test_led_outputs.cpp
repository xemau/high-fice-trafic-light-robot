#include "hardware/Esp32Adapters.h"
#include <cassert>
#include <vector>

struct Event { char operation; uint32_t first, second, third; };
std::vector<Event> events;
HardwareSerial Serial;
bool pwmSetupOk = true;

uint32_t millis() { return 0; }
void pinMode(uint8_t pin, uint8_t mode) { events.push_back({'m', pin, mode, 0}); }
void digitalWrite(uint8_t pin, uint8_t value) { events.push_back({'w', pin, value, 0}); }
int digitalRead(uint8_t) { return HIGH; }
uint32_t ledcSetup(uint8_t channel, uint32_t frequency, uint8_t resolutionBits) {
    events.push_back({'s', channel, frequency, resolutionBits});
    return pwmSetupOk ? frequency : 0;
}
void ledcAttachPin(uint8_t pin, uint8_t channel) { events.push_back({'a', pin, channel, 0}); }
void ledcWrite(uint8_t channel, uint32_t duty) { events.push_back({'p', channel, duty, 0}); }

uint32_t expectedDuty(uint8_t brightness) {
    const uint32_t corrected = (static_cast<uint32_t>(brightness) * brightness + 127) / 255;
    return 255 - corrected;
}

int main() {
    constexpr uint8_t expectedPins[] = {18, 19, 23, 25, 26, 32, 33, 21, 22};
    Esp32LedOutputs outputs;
    assert(outputs.begin());
    assert(events.size() == 45);
    for (unsigned i = 0; i < 9; ++i) {
        const auto offset = i * 5;
        assert(events[offset].operation == 'w' && events[offset].first == expectedPins[i]);
        assert(events[offset].second == HIGH);
        assert(events[offset + 1].operation == 'm' && events[offset + 1].first == expectedPins[i]);
        assert(events[offset + 1].second == OUTPUT);
        assert(events[offset + 2].operation == 's' && events[offset + 2].first == i);
        assert(events[offset + 2].second == Config::LedPwmHz);
        assert(events[offset + 2].third == Config::LedPwmBits);
        assert(events[offset + 3].operation == 'a' && events[offset + 3].first == expectedPins[i]);
        assert(events[offset + 3].second == i);
        assert(events[offset + 4].operation == 'p' && events[offset + 4].first == i);
        assert(events[offset + 4].second == 255);
    }

    const uint8_t levels[] = {0, 1, 32, 64, 96, 128, 192, 254, 255};
    LedFrame frame{};
    for (unsigned pair = 0; pair < 3; ++pair) {
        frame[pair] = {levels[pair * 3], levels[pair * 3 + 1], levels[pair * 3 + 2]};
    }
    events.clear();
    outputs.write(frame);
    assert(events.size() == 9);
    for (unsigned i = 0; i < 9; ++i) {
        assert(events[i].operation == 'p' && events[i].first == i);
        assert(events[i].second == expectedDuty(levels[i]));
    }

    events.clear();
    pwmSetupOk = false;
    Esp32LedOutputs failed;
    assert(!failed.begin());
    assert(events.size() == 3);
    assert(events.back().operation == 's');
}

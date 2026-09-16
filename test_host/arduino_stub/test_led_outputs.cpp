#include "hardware/Esp32Adapters.h"
#include <cassert>
#include <vector>

struct Event { char operation; uint8_t pin, value; };
std::vector<Event> events;
HardwareSerial Serial;
uint32_t millis() { return 0; }
void pinMode(uint8_t pin, uint8_t mode) { events.push_back({'m', pin, mode}); }
void digitalWrite(uint8_t pin, uint8_t value) { events.push_back({'w', pin, value}); }
int digitalRead(uint8_t) { return HIGH; }

int main() {
    constexpr uint8_t expectedPins[] = {18, 19, 23, 25, 26, 32, 33, 21, 22};
    Esp32LedOutputs outputs;
    assert(outputs.begin());
    assert(events.size() == 18);
    for (unsigned i = 0; i < 9; ++i) {
        assert(events[2 * i].operation == 'w' && events[2 * i].pin == expectedPins[i]);
        assert(events[2 * i].value == HIGH);
        assert(events[2 * i + 1].operation == 'm' && events[2 * i + 1].pin == expectedPins[i]);
        assert(events[2 * i + 1].value == OUTPUT);
    }
    // Exercise every combination, including mixed colors and all channels off/on.
    for (unsigned mask = 0; mask < 512; ++mask) {
        LedFrame frame{};
        for (unsigned pair = 0; pair < 3; ++pair) {
            frame[pair] = {bool(mask & (1u << (pair * 3))),
                           bool(mask & (1u << (pair * 3 + 1))),
                           bool(mask & (1u << (pair * 3 + 2)))};
        }
        events.clear();
        outputs.write(frame);
        assert(events.size() >= 9);
        for (unsigned i = 0; i < 9; ++i) {
            assert(events[i].operation == 'w' && events[i].pin == expectedPins[i]);
            assert(events[i].value == HIGH);
        }
        unsigned next = 9;
        for (unsigned i = 0; i < 9; ++i) if (mask & (1u << i)) {
            assert(next < events.size());
            assert(events[next].operation == 'w' && events[next].pin == expectedPins[i]);
            assert(events[next++].value == LOW);
        }
        assert(next == events.size());
    }
}

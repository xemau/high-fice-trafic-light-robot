#include "core/DiagnosticController.h"
#include "core/HighFiveSensor.h"
#include "core/TrafficLight.h"
#include "core/YX5200AudioPlayer.h"
#include "hardware/Esp32Adapters.h"
#include <esp_system.h>
#include <cstdio>

namespace {
struct Application {
    Esp32Clock clock;
    SerialConsole console;
    Esp32Input input;
    Esp32Uart uart;
    Esp32LedOutputs ledOutputs;
    HighFiveSensor sensor{clock, input};
    TrafficLight lights{clock, ledOutputs};
    YX5200AudioPlayer audio{clock, uart, console};
    DiagnosticController diagnostics{clock, sensor, audio, lights, console};
    BootMenu menu{clock, diagnostics, console};
};
Application& app() {
    static Application instance;
    return instance;
}
}

void setup() {
    app().console.begin();
    char message[144];
    std::snprintf(message, sizeof(message), "[BOOT] reset_reason=%d free_heap=%lu build=%s %s",
                  static_cast<int>(esp_reset_reason()), static_cast<unsigned long>(ESP.getFreeHeap()), __DATE__, __TIME__);
    app().console.log(message);
    app().menu.begin();
}

void loop() {
    auto& instance = app();
    for (std::size_t i = 0; i < Config::IoBudget; ++i) {
        const int byte = instance.console.read();
        if (byte < 0) break;
        instance.menu.input(static_cast<char>(byte));
    }
    instance.menu.update();
    instance.console.update();
}

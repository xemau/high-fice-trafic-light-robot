#include "core/DiagnosticController.h"
#include "core/HighFiveSensor.h"
#include "core/TrafficLight.h"
#include "core/YX5200AudioPlayer.h"
#include "hardware/Esp32Adapters.h"
#include <esp_system.h>

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
    LineBuffer line;
};
Application& app() {
    static Application instance;
    return instance;
}
}

void setup() {
    const uint32_t seed = esp_random();
    app().lights.seedAnimation(seed);
    app().diagnostics.seedRandom(esp_random() ^ (seed << 1));
    app().console.begin();
    app().diagnostics.begin(AppMode::Full);
}

void loop() {
    auto& instance = app();
    for (std::size_t i = 0; i < Config::IoBudget; ++i) {
        const int byte = instance.console.read();
        if (byte < 0) break;
        const auto result = instance.line.push(static_cast<char>(byte));
        if (result == LineResult::Rejected) {
            instance.console.log("[ERROR] input line too long or contains invalid bytes");
        } else if (result == LineResult::Complete) {
            instance.diagnostics.command(parseCommand(instance.line.text()));
        }
    }
    instance.diagnostics.update();
    instance.console.update();
}

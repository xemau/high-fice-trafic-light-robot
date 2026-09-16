#include "core/DiagnosticController.h"
#include "core/Timing.h"
#include <cstdio>

bool DiagnosticController::robotMode() const { return mode_ == AppMode::Full || mode_ == AppMode::SequenceTest; }
bool DiagnosticController::audioMode() const { return robotMode() || mode_ == AppMode::AudioTest; }
bool DiagnosticController::lightsMode() const { return robotMode() || mode_ == AppMode::LightsTest; }

void DiagnosticController::begin(AppMode mode) {
    mode_ = mode;
    active_ = true;
    cycle_ = false;
    const char* name = "FULL";
    switch (mode_) {
        case AppMode::Full: break;
        case AppMode::LightsTest: name = "LIGHTS TEST"; break;
        case AppMode::AudioTest: name = "AUDIO TEST"; break;
        case AppMode::SensorTest: name = "SENSOR TEST"; break;
        case AppMode::SequenceTest: name = "SEQUENCE TEST (virtual hardware)"; break;
        default: mode_ = AppMode::Full; break;
    }
    bootPending_ = mode_ == AppMode::Full;
    errorPending_ = false;
    char message[64];
    std::snprintf(message, sizeof(message), "[MODE] %s", name);
    log_.log(message);
    if (lightsMode()) {
        const bool ok = lights().begin();
        log_.log(ok ? "[OK] LEDs (driver initialized)" : "[ERROR] LEDs initialization failed");
        errorPending_ = !ok;
    }
    if (robotMode() || mode_ == AppMode::SensorTest) {
        const bool ok = sensor().begin();
        log_.log(ok ? "[OK] sensor" : "[ERROR] sensor initialization failed");
        errorPending_ = errorPending_ || !ok;
        wasPressed_ = sensor().pressed();
    }
    if (audioMode() && !audio().begin()) log_.log("[ERROR] audio initialization failed");
    if (robotMode()) {
        robot().begin();
        log_.log("[OK] robot ready (audio may still be starting)");
    } else if (mode_ == AppMode::LightsTest) {
        cycle_ = true;
        cycledAt_ = clock_.now();
        cycleStep_ = 0;
        lights_.show(Lamp::Red);
    } else if (mode_ == AppMode::SensorTest) {
        log_.log(wasPressed_ ? "[SENSOR] PRESSED" : "[SENSOR] RELEASED");
    }
    help();
}

void DiagnosticController::update() {
    if (!active_) return;
    if (robotMode()) {
        robot().update();
    } else if (mode_ == AppMode::LightsTest) {
        const auto now = clock_.now();
        if (cycle_ && elapsed(now, cycledAt_, Config::LightCycleMs)) {
            cycledAt_ = now;
            cycleStep_ = (cycleStep_ + 1) % 4;
            lights_.show(static_cast<Lamp>(cycleStep_));
        }
        lights_.updateAnimation();
    } else if (mode_ == AppMode::AudioTest) {
        audio_.update();
    } else if (mode_ == AppMode::SensorTest) {
        sensor_.update();
        if (sensor_.pressed() != wasPressed_) {
            wasPressed_ = sensor_.pressed();
            log_.log(wasPressed_ ? "[SENSOR] PRESSED" : "[SENSOR] RELEASED");
        }
        if (sensor_.takePress()) log_.log("[SENSOR] HIGH FIVE EVENT");
    }
    if (audioMode()) updateSounds();
}

void DiagnosticController::signalError() {
    if (audioMode() && audio().status() == AudioStatus::Ready) {
        if (!audio().playTrack(Config::ErrorTrack)) log_.log("[ERROR] error sound unavailable");
    }
}

void DiagnosticController::updateSounds() {
    if (audio().takeError()) errorPending_ = true;
    if (audio().status() == AudioStatus::Failed) {
        bootPending_ = errorPending_ = false;
        return;
    }
    if (audio().status() != AudioStatus::Ready) return;
    if (errorPending_) {
        signalError();
        bootPending_ = errorPending_ = false;
    } else if (bootPending_) {
        bootPending_ = false;
        // A late audio startup must not interrupt an already-started reward.
        if (robotState() != RobotState::Reward) {
            if (!audio().playTrack(Config::BootTrack)) log_.log("[ERROR] boot sound unavailable");
        }
    }
}

void DiagnosticController::help() {
    log_.log("[HELP] status | help; reboot to select another mode");
    if (lightsMode()) log_.log("[HELP] red | yellow | green | off | lights <color>");
    if (mode_ == AppMode::LightsTest) log_.log("[HELP] cycle | dance");
    if (audioMode()) log_.log("[HELP] play <1-9999> | stop | pause | resume | volume <0-30> | next | previous | retry");
    if (audioMode()) log_.log("[HELP] boot | error (system sound tests)");
    if (robotMode()) log_.log("[HELP] highfive | simulate highfive | reset");
}

void DiagnosticController::reportStatus() {
    char message[128];
    const char* audio = "unused";
    if (audioMode()) {
        switch (this->audio().status()) {
            case AudioStatus::Off: audio = "off"; break;
            case AudioStatus::Starting: audio = "starting"; break;
            case AudioStatus::Ready: audio = "ready"; break;
            case AudioStatus::Failed: audio = "failed"; break;
        }
    }
    std::snprintf(message, sizeof(message), "[STATUS] mode=%u state=%s audio=%s sensor=%s",
                  static_cast<unsigned>(mode_), robotMode() ? RobotController::stateName(robotState()) : "unused",
                  audio, robotMode() || mode_ == AppMode::SensorTest ? (sensor().pressed() ? "pressed" : "released") : "unused");
    log_.log(message);
}

void DiagnosticController::command(Command cmd) {
    if (!active_) return;
    if (cmd.type == CommandType::Help) { help(); return; }
    if (cmd.type == CommandType::Status) { reportStatus(); return; }
    if (robotMode() && cmd.type == CommandType::HighFive) { robot().simulateHighFive(); return; }
    if (robotMode() && cmd.type == CommandType::Reset) { robot().begin(); return; }
    if (lightsMode()) {
        Lamp lamp = Lamp::Off;
        bool isLamp = true;
        switch (cmd.type) {
            case CommandType::Red: lamp = Lamp::Red; break;
            case CommandType::Yellow: lamp = Lamp::Yellow; break;
            case CommandType::Green: lamp = Lamp::Green; break;
            case CommandType::Off: break;
            default: isLamp = false; break;
        }
        if (isLamp) {
            cycle_ = false;
            lights().show(lamp);
            log_.log("[LIGHTS] manual display (robot restores on next state transition)");
            return;
        }
    }
    if (mode_ == AppMode::LightsTest && (cmd.type == CommandType::Cycle || cmd.type == CommandType::Dance)) {
        cycle_ = cmd.type == CommandType::Cycle;
        if (cycle_) {
            cycleStep_ = 0;
            cycledAt_ = clock_.now();
            lights_.show(Lamp::Red);
        } else lights_.startAnimation();
        log_.log(cycle_ ? "[LIGHTS] cycle" : "[LIGHTS] RGB pair animation");
        return;
    }
    if (audioMode()) {
        bool ok = false, handled = true;
        switch (cmd.type) {
            case CommandType::Play: ok = audio().playTrack(static_cast<uint16_t>(cmd.value)); break;
            case CommandType::Stop: ok = audio().stop(); break;
            case CommandType::Pause: ok = audio().pause(); break;
            case CommandType::Resume: ok = audio().resume(); break;
            case CommandType::Volume: ok = audio().setVolume(cmd.value); break;
            case CommandType::Next: ok = audio().next(); break;
            case CommandType::Previous: ok = audio().previous(); break;
            case CommandType::Retry: ok = audio().begin(); break;
            case CommandType::BootSound: ok = audio().playTrack(Config::BootTrack); break;
            case CommandType::ErrorSound: ok = audio().playTrack(Config::ErrorTrack); break;
            default: handled = false; break;
        }
        if (handled) {
            log_.log(ok ? "[AUDIO] accepted (asynchronous; not proof of sound)" : "[ERROR] audio unavailable, queue full, or invalid argument");
            return;
        }
    }
    log_.log("[ERROR] invalid command or unavailable in this mode; type help");
    signalError();
}

void BootMenu::begin() {
    startedAt_ = clock_.now();
    selected_ = false;
    line_ = LineBuffer{};
    log_.log("[BOOT] High-Five Robot");
    log_.log("1 - Full robot\n2 - Lights test\n3 - Audio test\n4 - High-five sensor test\n5 - Sequence/state-machine test");
    log_.log("Select mode (number + Enter); timeout starts configured default:");
}

void BootMenu::update() {
    if (!selected_ && elapsed(clock_.now(), startedAt_, timeout_)) {
        selected_ = true;
        line_ = LineBuffer{};
        diagnostics_.begin(default_);
    }
    if (selected_) diagnostics_.update();
}

void BootMenu::input(char c) {
    const auto result = line_.push(c);
    if (result == LineResult::Rejected) log_.log("[ERROR] input line too long or contains invalid bytes");
    if (result != LineResult::Complete) return;
    const auto cmd = parseCommand(line_.text());
    if (selected_) diagnostics_.command(cmd);
    else if (cmd.type == CommandType::SelectMode) {
        selected_ = true;
        diagnostics_.begin(static_cast<AppMode>(cmd.value));
    } else log_.log("[ERROR] select 1..5 followed by Enter");
}

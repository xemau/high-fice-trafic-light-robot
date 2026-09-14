#pragma once
#include "core/CommandParser.h"
#include "core/RobotController.h"
#include "core/VirtualHardware.h"

class DiagnosticController {
public:
    DiagnosticController(IClock& clock, IHighFiveSensor& sensor, IAudioPlayer& audio,
                         ITrafficLight& lights, ILogger& log)
        : clock_(clock), sensor_(sensor), audio_(audio), lights_(lights), log_(log),
          robot_(clock, sensor, audio, lights, log),
          sequence_(clock, virtualSensor_, virtualAudio_, virtualLights_, log) {}
    void begin(AppMode mode);
    void update();
    void command(Command command);
    AppMode mode() const { return mode_; }
    RobotState robotState() const { return mode_ == AppMode::SequenceTest ? sequence_.state() : robot_.state(); }
private:
    bool robotMode() const;
    bool audioMode() const;
    bool lightsMode() const;
    void help();
    void reportStatus();
    IHighFiveSensor& sensor() { return mode_ == AppMode::SequenceTest ? virtualSensor_ : sensor_; }
    IAudioPlayer& audio() { return mode_ == AppMode::SequenceTest ? virtualAudio_ : audio_; }
    ITrafficLight& lights() { return mode_ == AppMode::SequenceTest ? virtualLights_ : lights_; }
    RobotController& robot() { return mode_ == AppMode::SequenceTest ? sequence_ : robot_; }
    IClock& clock_;
    IHighFiveSensor& sensor_;
    IAudioPlayer& audio_;
    ITrafficLight& lights_;
    ILogger& log_;
    RobotController robot_;
    VirtualSensor virtualSensor_;
    VirtualAudio virtualAudio_;
    VirtualLights virtualLights_;
    RobotController sequence_;
    AppMode mode_ = AppMode::Full;
    bool active_ = false, cycle_ = false, wasPressed_ = false;
    uint32_t cycledAt_ = 0;
    uint8_t cycleStep_ = 0;
};

class BootMenu {
public:
    BootMenu(IClock& clock, DiagnosticController& diagnostics, ILogger& log,
             uint32_t timeout = Config::SelectionMs, AppMode defaultMode = Config::DefaultMode)
        : clock_(clock), diagnostics_(diagnostics), log_(log), timeout_(timeout), default_(defaultMode) {}
    void begin();
    void update();
    void input(char c);
    bool selected() const { return selected_; }
private:
    IClock& clock_;
    DiagnosticController& diagnostics_;
    ILogger& log_;
    LineBuffer line_;
    uint32_t timeout_, startedAt_ = 0;
    AppMode default_;
    bool selected_ = false;
};

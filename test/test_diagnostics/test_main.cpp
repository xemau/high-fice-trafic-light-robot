#include <unity.h>
#include "core/DiagnosticController.h"
#include "core/HighFiveSensor.h"
#include "core/TrafficLight.h"
#include "../support/Fakes.h"
#include <cstring>

void setUp() {}
void tearDown() {}
struct Rig {
    FakeClock clock; FakeSensor sensor; FakeAudio audio; FakeLights lights; FakeLog log;
    DiagnosticController diagnostics{clock, sensor, audio, lights, log};
    BootMenu menu{clock, diagnostics, log};
    void command(const char* text) { diagnostics.command(parseCommand(text)); }
    void input(const char* text) { for (const char* p = text; *p; ++p) menu.input(*p); }
    void green() { clock.advance(Config::RedMs); diagnostics.update(); clock.advance(Config::YellowMs); diagnostics.update(); }
};
void parser_valid_commands() {
    struct Case { const char* text; CommandType type; int value; };
    const Case cases[] = {
        {"red", CommandType::Red, 0}, {"yellow", CommandType::Yellow, 0}, {"green", CommandType::Green, 0},
        {"off", CommandType::Off, 0}, {"lights red", CommandType::Red, 0}, {"lights yellow", CommandType::Yellow, 0},
        {"lights green", CommandType::Green, 0}, {"lights off", CommandType::Off, 0},
        {"cycle", CommandType::Cycle, 0}, {"dance", CommandType::Dance, 0},
        {"play 1", CommandType::Play, 1}, {"play 9999", CommandType::Play, 9999},
        {"volume 0", CommandType::Volume, 0}, {"volume 30", CommandType::Volume, 30},
        {" \tvolume\t20 \t", CommandType::Volume, 20},
        {"stop", CommandType::Stop, 0}, {"pause", CommandType::Pause, 0}, {"resume", CommandType::Resume, 0},
        {"next", CommandType::Next, 0}, {"previous", CommandType::Previous, 0},
        {"highfive", CommandType::HighFive, 0}, {"simulate highfive", CommandType::HighFive, 0},
        {"status", CommandType::Status, 0}, {"reset", CommandType::Reset, 0}, {"help", CommandType::Help, 0},
        {"retry", CommandType::Retry, 0}, {"1", CommandType::SelectMode, 1}, {"5", CommandType::SelectMode, 5},
        {"boot", CommandType::BootSound, 0}, {"error", CommandType::ErrorSound, 0}
    };
    for (const auto& c : cases) {
        const auto command = parseCommand(c.text);
        TEST_ASSERT_EQUAL_INT_MESSAGE(c.type, command.type, c.text);
        TEST_ASSERT_EQUAL_MESSAGE(c.value, command.value, c.text);
    }
}
void parser_rejects_malformed_input() {
    const char* invalid[] = {nullptr, "", " ", "red green", "play", "play -1", "play 0", "play 10000",
        "play 99999999999999999999999999999", "play 1x", "play 1 2", "volume 31", "volume -1", "volume +1",
        "volume", "lights blue", "lights red green", "simulate press", "0", "6", "11", "wat", "RED", "red\n", "\x01"};
    for (auto c : invalid) TEST_ASSERT_EQUAL_INT(CommandType::Invalid, parseCommand(c).type);
    std::string huge(200, 'a'); TEST_ASSERT_EQUAL_INT(CommandType::Invalid, parseCommand(huge.c_str()).type);
}
void line_buffer_crlf_backspace_overflow_and_recovery() {
    LineBuffer line;
    TEST_ASSERT_EQUAL_INT(LineResult::Pending, line.push('\n'));
    line.push('\b'); line.push('r'); line.push('e'); line.push('x'); line.push('\b'); line.push('d');
    TEST_ASSERT_EQUAL_INT(LineResult::Complete, line.push('\r')); TEST_ASSERT_EQUAL_STRING("red", line.text());
    TEST_ASSERT_EQUAL_INT(LineResult::Pending, line.push('\n'));
    for (std::size_t i = 0; i < Config::SerialLineSize - 1; ++i) line.push('x');
    TEST_ASSERT_EQUAL_INT(LineResult::Complete, line.push('\n'));
    TEST_ASSERT_EQUAL(Config::SerialLineSize - 1, std::strlen(line.text()));
    for (std::size_t i = 0; i < Config::SerialLineSize + 10; ++i) line.push('x');
    line.push('\b'); TEST_ASSERT_EQUAL_INT(LineResult::Rejected, line.push('\n'));
    line.push('x'); line.push(127); line.push('\t'); line.push('5');
    TEST_ASSERT_EQUAL_INT(LineResult::Complete, line.push('\n')); TEST_ASSERT_EQUAL_STRING("\t5", line.text());
    line.push('\0'); TEST_ASSERT_EQUAL_INT(LineResult::Rejected, line.push('\n'));
    line.push(static_cast<char>(0xff)); TEST_ASSERT_EQUAL_INT(LineResult::Rejected, line.push('\n'));
}
void boot_default_boundary_and_no_serial_dependency() {
    Rig r; r.menu.begin();
    r.clock.advance(Config::SelectionMs - 1); r.menu.update(); TEST_ASSERT_FALSE(r.menu.selected());
    TEST_ASSERT_EQUAL(0, r.sensor.begins); TEST_ASSERT_EQUAL(0, r.audio.begins); TEST_ASSERT_EQUAL(0, r.lights.begins);
    r.clock.advance(1); r.menu.update(); TEST_ASSERT_TRUE(r.menu.selected());
    TEST_ASSERT_EQUAL_INT(AppMode::Full, r.diagnostics.mode());
    TEST_ASSERT_EQUAL(1, r.sensor.begins); TEST_ASSERT_EQUAL(1, r.audio.begins); TEST_ASSERT_EQUAL(1, r.lights.begins);
    r.input("status\r\n"); TEST_ASSERT_TRUE(r.log.contains("mode=1 state=RED audio=ready"));
}
void boot_selection_invalid_input_timeout_and_wrap() {
    Rig r; r.menu.begin(); r.input("no\n"); TEST_ASSERT_FALSE(r.menu.selected());
    r.input("2\r\n"); TEST_ASSERT_TRUE(r.menu.selected()); TEST_ASSERT_EQUAL_INT(AppMode::LightsTest, r.diagnostics.mode());
    r.input("red\n"); TEST_ASSERT_EQUAL_INT(Lamp::Red, r.lights.lamp);
    r.input(std::string(100, 'x').c_str()); r.input("\n"); TEST_ASSERT_TRUE(r.log.contains("input line too long"));
    Rig wrapped; wrapped.clock.time = UINT32_MAX - 10;
    BootMenu menu(wrapped.clock, wrapped.diagnostics, wrapped.log, 30, AppMode::SensorTest);
    menu.begin(); menu.input('2'); wrapped.clock.advance(29); menu.update(); TEST_ASSERT_FALSE(menu.selected());
    wrapped.clock.advance(2); menu.update(); TEST_ASSERT_TRUE(menu.selected());
    TEST_ASSERT_EQUAL_INT(AppMode::SensorTest, wrapped.diagnostics.mode());
    menu.input('\n'); TEST_ASSERT_FALSE(wrapped.log.contains("invalid command"));
}
void lights_mode_isolated_and_cycles_with_manual_override() {
    Rig r; r.audio.ok = false; r.sensor.ok = false;
    r.diagnostics.begin(AppMode::LightsTest);
    TEST_ASSERT_EQUAL(0, r.audio.begins); TEST_ASSERT_EQUAL(0, r.sensor.begins); TEST_ASSERT_EQUAL(1, r.lights.begins);
    r.clock.advance(Config::LightCycleMs - 1); r.diagnostics.update(); TEST_ASSERT_EQUAL_INT(Lamp::Red, r.lights.lamp);
    for (int i = 1; i <= 4; ++i) {
        r.clock.advance(i == 1 ? 1 : Config::LightCycleMs); r.diagnostics.update();
        TEST_ASSERT_EQUAL_INT(i % 4, r.lights.lamp);
    }
    const char* commands[] = {"red", "yellow", "green", "off"};
    for (int i = 0; i < 4; ++i) {
        r.command(commands[i]); r.clock.advance(Config::LightCycleMs); r.diagnostics.update();
        TEST_ASSERT_EQUAL_INT(i, r.lights.lamp);
    }
    r.command("dance"); TEST_ASSERT_TRUE(r.lights.dancing); r.diagnostics.update();
    r.command("cycle"); TEST_ASSERT_FALSE(r.lights.dancing); TEST_ASSERT_EQUAL_INT(Lamp::Red, r.lights.lamp);
    r.command("play 1"); r.command("highfive"); r.command("reset"); r.command("status"); r.command("help");
    TEST_ASSERT_TRUE(r.log.contains("audio=unused sensor=unused"));
    TEST_ASSERT_EQUAL(0, r.audio.updates); TEST_ASSERT_EQUAL(0, r.sensor.updates);
    TEST_ASSERT_EQUAL(0, r.audio.plays);
}
void audio_mode_isolated_all_commands_and_errors() {
    Rig r; r.sensor.ok = false; r.lights.ok = false; r.diagnostics.begin(AppMode::AudioTest);
    TEST_ASSERT_EQUAL(0, r.sensor.begins); TEST_ASSERT_EQUAL(0, r.lights.begins);
    const char* commands[] = {"play 2", "volume 20", "stop", "pause", "resume", "next", "previous"};
    for (auto c : commands) { r.command(c); r.diagnostics.update(); }
    TEST_ASSERT_EQUAL(2, r.audio.track); TEST_ASSERT_EQUAL(20, r.audio.volume);
    TEST_ASSERT_EQUAL(1, r.audio.stops); TEST_ASSERT_EQUAL(1, r.audio.pauses); TEST_ASSERT_EQUAL(1, r.audio.resumes);
    TEST_ASSERT_EQUAL(1, r.audio.nexts); TEST_ASSERT_EQUAL(1, r.audio.previouses);
    TEST_ASSERT_EQUAL(0, r.lights.updates); TEST_ASSERT_EQUAL(0, r.sensor.updates);
    r.command("retry"); TEST_ASSERT_EQUAL(2, r.audio.begins);
    r.audio.ok = false; r.command("play 1"); TEST_ASSERT_TRUE(r.log.contains("audio unavailable"));
    r.command("red"); r.command("what"); TEST_ASSERT_TRUE(r.log.contains("invalid command"));
    for (auto s : {AudioStatus::Off, AudioStatus::Starting, AudioStatus::Ready, AudioStatus::Failed}) { r.audio.state = s; r.command("status"); }
    TEST_ASSERT_TRUE(r.log.contains("audio=off")); TEST_ASSERT_TRUE(r.log.contains("audio=starting"));
    TEST_ASSERT_TRUE(r.log.contains("audio=ready")); TEST_ASSERT_TRUE(r.log.contains("audio=failed"));
}
void sensor_mode_isolated_changes_only() {
    Rig r; r.audio.ok = false; r.lights.ok = false; r.diagnostics.begin(AppMode::SensorTest);
    TEST_ASSERT_EQUAL(0, r.audio.begins); TEST_ASSERT_EQUAL(0, r.lights.begins);
    const auto count = r.log.messages.size();
    for (int i = 0; i < 100; ++i) r.diagnostics.update();
    TEST_ASSERT_EQUAL(count, r.log.messages.size());
    r.sensor.down = true; r.sensor.event = true; r.diagnostics.update();
    TEST_ASSERT_TRUE(r.log.contains("PRESSED")); TEST_ASSERT_TRUE(r.log.contains("HIGH FIVE EVENT"));
    r.command("status"); TEST_ASSERT_TRUE(r.log.contains("sensor=pressed"));
    r.sensor.down = false; r.diagnostics.update(); TEST_ASSERT_TRUE(r.log.contains("RELEASED"));
    r.command("play 1"); r.command("red"); TEST_ASSERT_EQUAL(0, r.audio.plays); TEST_ASSERT_EQUAL(0, r.lights.shows);
    Rig held; held.sensor.down = true; held.diagnostics.begin(AppMode::SensorTest); TEST_ASSERT_TRUE(held.log.contains("PRESSED"));
}
void sequence_uses_no_physical_subsystems() {
    Rig r; r.audio.ok = r.sensor.ok = r.lights.ok = false;
    r.diagnostics.begin(AppMode::SequenceTest); r.green(); r.command("highfive"); r.diagnostics.update();
    TEST_ASSERT_EQUAL_INT(RobotState::Reward, r.diagnostics.robotState());
    TEST_ASSERT_TRUE(r.log.contains("REWARD"));
    r.command("status"); TEST_ASSERT_TRUE(r.log.contains("audio=ready"));
    r.clock.advance(Config::RewardMs); r.diagnostics.update(); TEST_ASSERT_EQUAL_INT(RobotState::Red, r.diagnostics.robotState());
    r.command("reset"); r.command("lights green"); r.command("pause");
    TEST_ASSERT_EQUAL(0, r.sensor.begins); TEST_ASSERT_EQUAL(0, r.sensor.updates); TEST_ASSERT_EQUAL(0, r.sensor.takes);
    TEST_ASSERT_EQUAL(0, r.audio.begins); TEST_ASSERT_EQUAL(0, r.audio.updates); TEST_ASSERT_EQUAL(0, r.audio.plays);
    TEST_ASSERT_EQUAL(0, r.lights.begins); TEST_ASSERT_EQUAL(0, r.lights.shows); TEST_ASSERT_EQUAL(0, r.lights.updates);
}
void full_failures_and_simulation_reset() {
    Rig r; r.audio.ok = r.sensor.ok = r.lights.ok = false;
    r.diagnostics.update(); r.command("status"); TEST_ASSERT_EQUAL(0, r.log.messages.size());
    r.diagnostics.begin(static_cast<AppMode>(99));
    TEST_ASSERT_EQUAL_INT(AppMode::Full, r.diagnostics.mode());
    TEST_ASSERT_TRUE(r.log.contains("LEDs initialization failed")); TEST_ASSERT_TRUE(r.log.contains("sensor initialization failed"));
    TEST_ASSERT_TRUE(r.log.contains("audio initialization failed"));
    r.green(); r.command("simulate highfive"); r.diagnostics.update();
    TEST_ASSERT_EQUAL_INT(RobotState::Reward, r.diagnostics.robotState());
    r.command("reset"); TEST_ASSERT_EQUAL_INT(RobotState::Red, r.diagnostics.robotState());
    TEST_ASSERT_EQUAL(1, r.audio.begins); TEST_ASSERT_EQUAL(1, r.sensor.begins); TEST_ASSERT_EQUAL(1, r.lights.begins);
}
void virtual_audio_public_contract() {
    VirtualAudio audio; TEST_ASSERT_EQUAL_INT(AudioStatus::Off, audio.status());
    TEST_ASSERT_FALSE(audio.playTrack(1)); TEST_ASSERT_FALSE(audio.setVolume(1));
    TEST_ASSERT_TRUE(audio.begin()); audio.update(); TEST_ASSERT_EQUAL_INT(AudioStatus::Ready, audio.status());
    TEST_ASSERT_TRUE(audio.playTrack(1)); TEST_ASSERT_FALSE(audio.playTrack(0)); TEST_ASSERT_FALSE(audio.playTrack(10000));
    TEST_ASSERT_TRUE(audio.stop()); TEST_ASSERT_TRUE(audio.pause()); TEST_ASSERT_TRUE(audio.resume());
    TEST_ASSERT_TRUE(audio.next()); TEST_ASSERT_TRUE(audio.previous());
    TEST_ASSERT_TRUE(audio.setVolume(0)); TEST_ASSERT_TRUE(audio.setVolume(30));
    TEST_ASSERT_FALSE(audio.setVolume(-1)); TEST_ASSERT_FALSE(audio.setVolume(31));
}
void full_integration_with_real_core_and_fake_electrical_io() {
    FakeClock clock; FakeInput input; FakeUart uart; FakeLedOutputs outputs; FakeLog log;
    HighFiveSensor sensor(clock, input); TrafficLight lights(clock, outputs); YX5200AudioPlayer audio(clock, uart, log);
    DiagnosticController diagnostics(clock, sensor, audio, lights, log);
    diagnostics.begin(AppMode::Full);
    clock.advance(Config::AudioBootMs); diagnostics.update();
    for (int i = 0; i < 5; ++i) { clock.advance(Config::AudioCommandMs); diagnostics.update(); }
    uart.respond(0x43, Config::DefaultVolume); diagnostics.update();
    clock.advance(Config::AudioCommandMs); diagnostics.update();
    clock.advance(Config::AudioCommandMs); diagnostics.update();
    TEST_ASSERT_EQUAL_INT(RobotState::GreenWaiting, diagnostics.robotState());
    input.level = false; diagnostics.update(); clock.advance(Config::DebounceMs); diagnostics.update();
    TEST_ASSERT_EQUAL_INT(RobotState::Reward, diagnostics.robotState());
    const auto rewardStartedAt = clock.now();
    clock.advance(Config::AudioCommandMs); diagnostics.update();
    TEST_ASSERT_EQUAL(0x06, uart.tx.back()[3]); TEST_ASSERT_EQUAL(Config::MusicVolume, uart.tx.back()[6]);
    clock.advance(Config::AudioCommandMs); diagnostics.update();
    TEST_ASSERT_EQUAL(0x12, uart.tx.back()[3]); TEST_ASSERT_EQUAL(Config::RewardTrack, uart.tx.back()[6]);
    while (clock.now() - rewardStartedAt < Config::RewardMs - 1) {
        const auto remaining = Config::RewardMs - 1 - (clock.now() - rewardStartedAt);
        clock.advance(remaining < 100 ? remaining : 100);
        uart.respond(0x42, 0x0201); diagnostics.update();
        TEST_ASSERT_EQUAL_INT(RobotState::Reward, diagnostics.robotState());
    }
    const auto beforeStop = uart.tx.size();
    clock.advance(1); diagnostics.update();
    TEST_ASSERT_EQUAL_INT(RobotState::Red, diagnostics.robotState());
    clock.advance(Config::AudioCommandMs); diagnostics.update();
    bool stopped = false;
    for (auto i = beforeStop; i < uart.tx.size(); ++i) stopped = stopped || uart.tx[i][3] == 0x16;
    TEST_ASSERT_TRUE(stopped);
    TEST_ASSERT_TRUE(outputs.frame[0].red);
    TEST_ASSERT_FALSE(outputs.frame[0].green); TEST_ASSERT_FALSE(outputs.frame[0].blue);
    for (std::size_t i = 1; i < outputs.frame.size(); ++i) {
        TEST_ASSERT_FALSE(outputs.frame[i].red);
        TEST_ASSERT_FALSE(outputs.frame[i].green); TEST_ASSERT_FALSE(outputs.frame[i].blue);
    }
}
void boot_sound_once_after_ready_and_diagnostic_system_commands() {
    Rig r; r.diagnostics.begin(AppMode::Full); r.audio.state = AudioStatus::Starting;
    r.diagnostics.update(); TEST_ASSERT_EQUAL(0, r.audio.plays);
    r.audio.state = AudioStatus::Ready; r.diagnostics.update();
    TEST_ASSERT_EQUAL(Config::BootTrack, r.audio.track); TEST_ASSERT_EQUAL(1, r.audio.plays);
    r.diagnostics.update(); TEST_ASSERT_EQUAL(1, r.audio.plays);
    r.command("boot"); TEST_ASSERT_EQUAL(Config::BootTrack, r.audio.track);
    r.command("error"); TEST_ASSERT_EQUAL(Config::ErrorTrack, r.audio.track);
    Rig test; test.diagnostics.begin(AppMode::AudioTest); test.diagnostics.update(); TEST_ASSERT_EQUAL(0, test.audio.plays);
    test.command("boot"); TEST_ASSERT_EQUAL(Config::BootTrack, test.audio.track);
    test.command("error"); TEST_ASSERT_EQUAL(Config::ErrorTrack, test.audio.track);
}
void error_cues_are_one_shot_and_unavailable_audio_does_not_recurse() {
    Rig r; r.lights.ok = false; r.diagnostics.begin(AppMode::Full); r.diagnostics.update();
    TEST_ASSERT_EQUAL(Config::ErrorTrack, r.audio.track); TEST_ASSERT_EQUAL(1, r.audio.plays);
    r.diagnostics.update(); TEST_ASSERT_EQUAL(1, r.audio.plays);
    r.audio.error = true; r.diagnostics.update(); TEST_ASSERT_EQUAL(2, r.audio.plays);
    r.diagnostics.update(); TEST_ASSERT_EQUAL(2, r.audio.plays);
    r.command("invalid"); TEST_ASSERT_EQUAL(3, r.audio.plays);
    r.audio.ok = false; r.audio.error = true; r.diagnostics.update(); TEST_ASSERT_TRUE(r.log.contains("error sound unavailable"));
    r.audio.state = AudioStatus::Failed; r.audio.error = true;
    const int played = r.audio.plays; r.diagnostics.update(); r.command("invalid");
    TEST_ASSERT_EQUAL(played, r.audio.plays);
    VirtualAudio virtualAudio; TEST_ASSERT_FALSE(virtualAudio.takeError());
}
void delayed_or_failed_boot_cue_does_not_interrupt_reward() {
    Rig r; r.diagnostics.begin(AppMode::Full); r.audio.state = AudioStatus::Starting;
    r.green(); r.command("highfive"); r.diagnostics.update();
    TEST_ASSERT_EQUAL(1, r.audio.plays);
    r.audio.state = AudioStatus::Ready; r.diagnostics.update(); TEST_ASSERT_EQUAL(1, r.audio.plays);
    Rig unavailable; unavailable.diagnostics.begin(AppMode::Full); unavailable.audio.ok = false;
    unavailable.diagnostics.update(); TEST_ASSERT_TRUE(unavailable.log.contains("boot sound unavailable"));
}
void sound_logs_explain_manual_automatic_and_suppressed_cues() {
    Rig r; r.diagnostics.begin(AppMode::Full); r.diagnostics.update();
    TEST_ASSERT_TRUE(r.log.contains("role=boot track=2998 reason=automatic startup"));
    r.command("error"); TEST_ASSERT_TRUE(r.log.contains("role=error track=2999 reason=manual error command"));
    r.command("boot"); TEST_ASSERT_TRUE(r.log.contains("reason=manual boot command"));
    r.command("invalid"); TEST_ASSERT_TRUE(r.log.contains("reason=invalid command or command unavailable"));
    r.audio.state = AudioStatus::Failed; r.command("error");
    TEST_ASSERT_TRUE(r.log.contains("result=suppressed: audio not ready"));
    Rig isolated; isolated.diagnostics.begin(AppMode::LightsTest); isolated.command("error");
    TEST_ASSERT_TRUE(isolated.log.contains("result=suppressed: mode has no audio"));
    TEST_ASSERT_EQUAL(0, isolated.audio.plays);
    Rig failed; failed.lights.ok = failed.sensor.ok = false; failed.diagnostics.begin(AppMode::Full);
    failed.diagnostics.update();
    TEST_ASSERT_TRUE(failed.log.contains("reason=LED initialization failed; sensor initialization failed;"));
    TEST_ASSERT_TRUE(failed.log.contains("reason=pending error takes priority"));
    failed.audio.error = true; failed.diagnostics.update();
    TEST_ASSERT_TRUE(failed.log.contains("reason=audio player error"));
}
void hardware_error_to_sound_is_traceable_without_recursion() {
    FakeClock clock; FakeSensor sensor; FakeUart uart; FakeLights lights; FakeLog log;
    YX5200AudioPlayer audio(clock, uart, log);
    DiagnosticController diagnostics(clock, sensor, audio, lights, log);
    diagnostics.begin(AppMode::AudioTest);
    clock.advance(Config::AudioBootMs); diagnostics.update();
    for (int i = 0; i < 5; ++i) { clock.advance(Config::AudioCommandMs); diagnostics.update(); }
    uart.respond(0x43, Config::DefaultVolume); diagnostics.update();
    diagnostics.command(parseCommand("play 1")); clock.advance(Config::AudioCommandMs); diagnostics.update();
    clock.advance(Config::AudioCommandMs); diagnostics.update();
    uart.respond(0x40, 6); diagnostics.update();
    TEST_ASSERT_TRUE(log.contains("role=error track=2999 reason=[ERROR] YX5200 module error 6"));
    clock.advance(Config::AudioCommandMs); diagnostics.update();
    TEST_ASSERT_EQUAL(0x06, uart.tx.back()[3]); TEST_ASSERT_EQUAL(Config::DefaultVolume, uart.tx.back()[6]);
    clock.advance(Config::AudioCommandMs); diagnostics.update();
    TEST_ASSERT_EQUAL(0x12, uart.tx.back()[3]);
    TEST_ASSERT_EQUAL(Config::ErrorTrack, (uart.tx.back()[5] << 8) | uart.tx.back()[6]);
    const auto sent = uart.tx.size();
    uart.respond(0x40, 6); diagnostics.update(); clock.advance(Config::AudioCommandMs); diagnostics.update();
    TEST_ASSERT_EQUAL(sent, uart.tx.size()); TEST_ASSERT_TRUE(log.contains("no recursive error cue"));
    diagnostics.command(parseCommand("status")); TEST_ASSERT_TRUE(log.contains("[YX5200 STATUS]"));
    uart.respond(0x3b, 2); diagnostics.update();
    TEST_ASSERT_TRUE(log.contains("reason=[ERROR] YX5200 SD card removed result=suppressed"));
    const auto logged = log.messages.size(); diagnostics.update(); TEST_ASSERT_EQUAL(logged, log.messages.size());
}
void boot_input_logs_explain_late_mode_selection_error_sound() {
    Rig r; r.menu.begin(); r.clock.advance(Config::SelectionMs); r.menu.update(); r.input("3\n");
    TEST_ASSERT_TRUE(r.log.contains("selection timeout"));
    TEST_ASSERT_TRUE(r.log.contains("phase=active input=\"3\""));
    TEST_ASSERT_TRUE(r.log.contains("role=error track=2999 reason=invalid command"));
    Rig selected; selected.menu.begin(); selected.input("3\n");
    TEST_ASSERT_TRUE(selected.log.contains("phase=boot-menu input=\"3\""));
}
void music_uses_maximum_and_system_sounds_restore_default_volume() {
    Rig r; r.diagnostics.begin(AppMode::AudioTest);
    r.command("play 1"); TEST_ASSERT_EQUAL(30, r.audio.volume);
    r.command("boot"); TEST_ASSERT_EQUAL(12, r.audio.volume);
    r.command("play 2"); TEST_ASSERT_EQUAL(30, r.audio.volume);
    r.command("error"); TEST_ASSERT_EQUAL(12, r.audio.volume);
    r.command("play 2998"); TEST_ASSERT_EQUAL(12, r.audio.volume);
    r.command("play 2999"); TEST_ASSERT_EQUAL(12, r.audio.volume);
    r.command("play 1"); r.command("volume 8"); TEST_ASSERT_EQUAL(8, r.audio.volume);
    r.command("play 1"); TEST_ASSERT_EQUAL(30, r.audio.volume);
}
int main() {
    UNITY_BEGIN(); RUN_TEST(parser_valid_commands); RUN_TEST(parser_rejects_malformed_input);
    RUN_TEST(line_buffer_crlf_backspace_overflow_and_recovery); RUN_TEST(boot_default_boundary_and_no_serial_dependency);
    RUN_TEST(boot_selection_invalid_input_timeout_and_wrap); RUN_TEST(lights_mode_isolated_and_cycles_with_manual_override);
    RUN_TEST(audio_mode_isolated_all_commands_and_errors); RUN_TEST(sensor_mode_isolated_changes_only);
    RUN_TEST(sequence_uses_no_physical_subsystems); RUN_TEST(full_failures_and_simulation_reset);
    RUN_TEST(virtual_audio_public_contract); RUN_TEST(full_integration_with_real_core_and_fake_electrical_io);
    RUN_TEST(boot_sound_once_after_ready_and_diagnostic_system_commands);
    RUN_TEST(error_cues_are_one_shot_and_unavailable_audio_does_not_recurse);
    RUN_TEST(delayed_or_failed_boot_cue_does_not_interrupt_reward);
    RUN_TEST(sound_logs_explain_manual_automatic_and_suppressed_cues);
    RUN_TEST(hardware_error_to_sound_is_traceable_without_recursion);
    RUN_TEST(boot_input_logs_explain_late_mode_selection_error_sound);
    RUN_TEST(music_uses_maximum_and_system_sounds_restore_default_volume);
    return UNITY_END();
}

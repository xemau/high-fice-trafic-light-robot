#include <unity.h>
#include "core/YX5200AudioPlayer.h"
#include "../support/Fakes.h"
void setUp() {}
void tearDown() {}
struct Rig {
    FakeClock clock; FakeUart uart; FakeLog log; YX5200AudioPlayer audio{clock, uart, log};
    void tick(uint32_t ms = Config::AudioCommandMs) { clock.advance(ms); audio.update(); }
    void query() {
        TEST_ASSERT_TRUE(audio.begin()); tick(Config::AudioBootMs);
        for (int i = 0; i < 4; ++i) tick();
    }
    void ready() { query(); uart.respond(0x43, Config::DefaultVolume); audio.update(); TEST_ASSERT_EQUAL_INT(AudioStatus::Ready, audio.status()); }
    void last(uint8_t cmd, uint16_t arg = 0) {
        const auto expected = encodeMp3Frame(cmd, arg);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(expected.data(), uart.tx.back().data(), 10);
    }
};
void protocol_known_frame_and_large_track() {
    const uint8_t expected[] = {0x7e, 0xff, 0x06, 0x06, 0, 0, 0x0f, 0xfe, 0xe6, 0xef};
    const auto volume = encodeMp3Frame(0x06, 15);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, volume.data(), 10);
    const auto large = encodeMp3Frame(0x12, 9999);
    Mp3Parser parser; Mp3Frame frame{};
    for (std::size_t i = 0; i < 9; ++i) TEST_ASSERT_FALSE(parser.push(large[i], frame));
    TEST_ASSERT_TRUE(parser.push(large[9], frame));
    TEST_ASSERT_EQUAL(0x12, frame.command); TEST_ASSERT_EQUAL(9999, frame.parameter);
}
void parser_rejects_corruption_and_resynchronizes() {
    for (std::size_t corrupt = 0; corrupt < 10; ++corrupt) {
        Mp3Parser parser; Mp3Frame frame{}; auto bytes = encodeMp3Frame(0x43, 12); bytes[corrupt] ^= 0x10;
        for (auto b : bytes) TEST_ASSERT_FALSE(parser.push(b, frame));
        TEST_ASSERT_FALSE(parser.push(0x7e, frame));
        for (auto b : encodeMp3Frame(0x42, 0x027e)) parser.push(b, frame);
        TEST_ASSERT_EQUAL(0x42, frame.command); TEST_ASSERT_EQUAL(0x027e, frame.parameter);
        parser.reset();
    }
    Mp3Parser parser; Mp3Frame frame{}; auto ack = encodeMp3Frame(0x41, 0);
    ack[4] = 1; --ack[8];
    for (auto b : ack) parser.push(b, frame);
    TEST_ASSERT_EQUAL(0x41, frame.command);
}
void startup_pacing_verifies_volume_not_just_ack() {
    Rig r; TEST_ASSERT_EQUAL_INT(AudioStatus::Off, r.audio.status()); r.audio.update();
    TEST_ASSERT_FALSE(r.audio.playTrack(1)); TEST_ASSERT_TRUE(r.audio.begin());
    r.tick(Config::AudioBootMs - 1); TEST_ASSERT_EQUAL(0, r.uart.tx.size());
    r.tick(1); r.last(0x16);
    r.tick(Config::AudioCommandMs - 1); TEST_ASSERT_EQUAL(1, r.uart.tx.size());
    r.tick(1); r.last(0x09, 2); r.tick(); r.last(0x06, Config::DefaultVolume);
    r.tick(); r.last(0x1a); r.tick(); r.last(0x43);
    r.uart.respond(0x41, 0); r.uart.respond(0x3f, 2); r.uart.respond(0x43, 30); r.audio.update();
    TEST_ASSERT_EQUAL_INT(AudioStatus::Starting, r.audio.status());
    r.uart.respond(0x43, Config::DefaultVolume); r.audio.update();
    TEST_ASSERT_EQUAL_INT(AudioStatus::Ready, r.audio.status()); TEST_ASSERT_TRUE(r.log.contains("[OK] YX5200"));
}
void all_commands_and_volume_validation() {
    Rig r; r.ready();
    TEST_ASSERT_TRUE(r.audio.playTrack(1)); r.tick(); r.last(0x12, 1);
    TEST_ASSERT_TRUE(r.audio.playTrack(9999)); r.tick(); r.last(0x12, 9999);
    TEST_ASSERT_FALSE(r.audio.playTrack(0)); TEST_ASSERT_FALSE(r.audio.playTrack(10000));
    TEST_ASSERT_TRUE(r.audio.pause()); r.tick(); r.last(0x0e);
    TEST_ASSERT_TRUE(r.audio.resume()); r.tick(); r.last(0x0d);
    TEST_ASSERT_TRUE(r.audio.next()); r.tick(); r.last(0x01);
    TEST_ASSERT_TRUE(r.audio.previous()); r.tick(); r.last(0x02);
    TEST_ASSERT_TRUE(r.audio.setVolume(0)); r.tick(); r.last(0x06, 0);
    TEST_ASSERT_TRUE(r.audio.setVolume(30)); r.tick(); r.last(0x06, 30);
    TEST_ASSERT_FALSE(r.audio.setVolume(-1)); TEST_ASSERT_FALSE(r.audio.setVolume(31)); TEST_ASSERT_FALSE(r.audio.setVolume(1000));
    TEST_ASSERT_TRUE(r.audio.stop()); r.tick(); r.last(0x16);
}
void queue_overflow_and_stop_priority() {
    Rig r; r.ready();
    for (std::size_t i = 0; i < Config::AudioQueueSize; ++i) TEST_ASSERT_TRUE(r.audio.playTrack(i + 1));
    TEST_ASSERT_FALSE(r.audio.pause());
    const auto count = r.uart.tx.size(); r.audio.update(); TEST_ASSERT_EQUAL(count, r.uart.tx.size());
    TEST_ASSERT_TRUE(r.audio.stop()); r.tick(); r.last(0x16);
    r.tick(); TEST_ASSERT_EQUAL(count + 1, r.uart.tx.size());
    for (int i = 0; i < 12; ++i) { TEST_ASSERT_TRUE(r.audio.playTrack(i + 1)); r.tick(); r.last(0x12, i + 1); }
}
void missing_module_timeout_retry_and_transport_failure() {
    Rig r; r.uart.ok = false; TEST_ASSERT_FALSE(r.audio.begin());
    TEST_ASSERT_EQUAL_INT(AudioStatus::Failed, r.audio.status()); r.audio.update();
    r.uart.ok = true; r.query(); r.tick(Config::AudioResponseMs - 1);
    TEST_ASSERT_EQUAL_INT(AudioStatus::Starting, r.audio.status()); r.tick(1);
    TEST_ASSERT_EQUAL_INT(AudioStatus::Failed, r.audio.status()); TEST_ASSERT_TRUE(r.log.contains("initialization failed"));
    r.ready(); r.uart.writeOk = false; r.audio.playTrack(1); r.tick();
    TEST_ASSERT_EQUAL_INT(AudioStatus::Failed, r.audio.status());
    r.uart.writeOk = true; r.audio.begin(); r.uart.writeOk = false; r.tick(Config::AudioBootMs);
    TEST_ASSERT_EQUAL_INT(AudioStatus::Failed, r.audio.status());
}
void device_error_sd_removal_finish_and_unrelated_frames() {
    Rig r; r.ready(); r.uart.respond(0x3d, 1); r.uart.respond(0x99, 0); r.audio.update();
    TEST_ASSERT_TRUE(r.log.contains("track finished")); TEST_ASSERT_EQUAL_INT(AudioStatus::Ready, r.audio.status());
    r.uart.respond(0x3b, 1); r.audio.update(); TEST_ASSERT_EQUAL_INT(AudioStatus::Ready, r.audio.status());
    r.uart.respond(0x3b, 2); r.audio.update(); TEST_ASSERT_EQUAL_INT(AudioStatus::Failed, r.audio.status());
    r.ready(); r.uart.respond(0x40, 3); r.audio.update(); TEST_ASSERT_EQUAL_INT(AudioStatus::Failed, r.audio.status());
    TEST_ASSERT_TRUE(r.log.contains("module error 3")); TEST_ASSERT_TRUE(r.audio.takeError());
    TEST_ASSERT_FALSE(r.audio.takeError());
}
void missing_track_keeps_error_sound_playable_without_recursion() {
    for (uint16_t code : {5, 6}) {
        Rig r; r.ready(); r.audio.playTrack(1); r.tick();
        r.uart.respond(0x40, code); r.audio.update();
        TEST_ASSERT_EQUAL_INT(AudioStatus::Ready, r.audio.status());
        TEST_ASSERT_TRUE(r.audio.takeError()); TEST_ASSERT_FALSE(r.audio.takeError());
        TEST_ASSERT_TRUE(r.audio.playTrack(Config::ErrorTrack)); r.tick(); r.last(0x12, Config::ErrorTrack);
        r.uart.respond(0x40, 6); r.audio.update();
        TEST_ASSERT_FALSE(r.audio.takeError()); TEST_ASSERT_EQUAL_INT(AudioStatus::Ready, r.audio.status());
    }
    Rig starting; starting.audio.begin(); starting.uart.respond(0x40, 6); starting.audio.update();
    TEST_ASSERT_EQUAL_INT(AudioStatus::Failed, starting.audio.status());
    TEST_ASSERT_TRUE(starting.audio.takeError());
}
void health_poll_response_and_disconnect() {
    Rig r; r.ready(); r.tick(Config::AudioPollMs); r.last(0x42);
    r.uart.respond(0x42, 0x0201); r.audio.update();
    r.tick(Config::AudioResponseMs); TEST_ASSERT_EQUAL_INT(AudioStatus::Ready, r.audio.status());
    r.tick(Config::AudioPollMs); r.last(0x42);
    r.tick(Config::AudioResponseMs - 1); TEST_ASSERT_EQUAL_INT(AudioStatus::Ready, r.audio.status());
    r.tick(1); TEST_ASSERT_EQUAL_INT(AudioStatus::Failed, r.audio.status());
    TEST_ASSERT_TRUE(r.log.contains("disconnected"));
}
void partial_frames_timeout_and_bounded_rx() {
    Rig r; r.query(); const auto bytes = encodeMp3Frame(0x43, Config::DefaultVolume);
    for (int i = 0; i < 5; ++i) r.uart.rx.push_back(bytes[i]); r.audio.update();
    r.tick(Config::FrameTimeoutMs);
    for (int i = 5; i < 10; ++i) r.uart.rx.push_back(bytes[i]); r.audio.update();
    TEST_ASSERT_EQUAL_INT(AudioStatus::Starting, r.audio.status());
    r.uart.respond(0x43, Config::DefaultVolume); r.audio.update();
    TEST_ASSERT_EQUAL_INT(AudioStatus::Ready, r.audio.status());
    for (int i = 0; i < 1000; ++i) r.uart.rx.push_back(0);
    const auto before = r.uart.rx.size(); r.audio.update();
    TEST_ASSERT_EQUAL(Config::IoBudget, before - r.uart.rx.size());
}
void startup_and_poll_rollover() {
    Rig r; r.clock.time = UINT32_MAX - 1000; r.ready();
    TEST_ASSERT_EQUAL_INT(AudioStatus::Ready, r.audio.status());
    r.clock.time = UINT32_MAX - 10; r.audio.begin(); r.tick(Config::AudioBootMs);
    TEST_ASSERT_EQUAL_INT(AudioStatus::Starting, r.audio.status());
    for (int i = 0; i < 4; ++i) r.tick();
    r.tick(Config::AudioResponseMs + 1); TEST_ASSERT_EQUAL_INT(AudioStatus::Failed, r.audio.status());
}
int main() {
    UNITY_BEGIN(); RUN_TEST(protocol_known_frame_and_large_track); RUN_TEST(parser_rejects_corruption_and_resynchronizes);
    RUN_TEST(startup_pacing_verifies_volume_not_just_ack); RUN_TEST(all_commands_and_volume_validation);
    RUN_TEST(queue_overflow_and_stop_priority); RUN_TEST(missing_module_timeout_retry_and_transport_failure);
    RUN_TEST(device_error_sd_removal_finish_and_unrelated_frames); RUN_TEST(health_poll_response_and_disconnect);
    RUN_TEST(partial_frames_timeout_and_bounded_rx); RUN_TEST(startup_and_poll_rollover);
    RUN_TEST(missing_track_keeps_error_sound_playable_without_recursion);
    return UNITY_END();
}

#pragma once
#include "Config.h"
#include "interfaces/Interfaces.h"
#include <array>

struct Mp3Frame { uint8_t command; uint16_t parameter; };
using Mp3Bytes = std::array<uint8_t, 10>;
Mp3Bytes encodeMp3Frame(uint8_t command, uint16_t parameter);
class Mp3Parser {
public:
    bool push(uint8_t byte, Mp3Frame& frame);
    void reset() { size_ = 0; }
private:
    Mp3Bytes bytes_{};
    std::size_t size_ = 0;
};

class YX5200AudioPlayer final : public IAudioPlayer {
public:
    YX5200AudioPlayer(IClock& clock, IUart& uart, ILogger& log)
        : clock_(clock), uart_(uart), log_(log) {}
    bool begin() override;
    void update() override;
    bool playTrack(uint16_t track) override;
    bool stop() override;
    bool pause() override;
    bool resume() override;
    bool setVolume(int volume) override;
    bool next() override;
    bool previous() override;
    AudioStatus status() const override { return status_; }
private:
    bool enqueue(uint8_t command, uint16_t parameter = 0);
    bool send(uint8_t command, uint16_t parameter = 0);
    void receive(const Mp3Frame& frame);
    void fail(const char* reason);
    IClock& clock_;
    IUart& uart_;
    ILogger& log_;
    Mp3Parser parser_;
    std::array<Mp3Frame, Config::AudioQueueSize> queue_{};
    std::size_t head_ = 0, count_ = 0;
    AudioStatus status_ = AudioStatus::Off;
    uint32_t startedAt_ = 0, lastSent_ = 0, lastByte_ = 0, queriedAt_ = 0;
    uint8_t initStep_ = 0;
    bool awaitingStatus_ = false;
};

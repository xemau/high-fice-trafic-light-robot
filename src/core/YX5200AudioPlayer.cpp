#include "core/YX5200AudioPlayer.h"
#include "core/Timing.h"
#include <cstdio>

namespace {
constexpr std::array<Mp3Frame, 6> Initialization{{
    {0x16, 0}, {0x09, 2}, {0x06, Config::DefaultVolume},
    {0x1a, 0}, {0x07, Config::DefaultEqualizer}, {0x43, 0}
}};
}

Mp3Bytes encodeMp3Frame(uint8_t command, uint16_t parameter) {
    Mp3Bytes bytes{{0x7e, 0xff, 0x06, command, 0,
                   static_cast<uint8_t>(parameter >> 8), static_cast<uint8_t>(parameter), 0, 0, 0xef}};
    uint16_t sum = 0;
    for (std::size_t i = 1; i <= 6; ++i) sum += bytes[i];
    const auto checksum = static_cast<uint16_t>(0 - sum);
    bytes[7] = static_cast<uint8_t>(checksum >> 8);
    bytes[8] = static_cast<uint8_t>(checksum);
    return bytes;
}

bool Mp3Parser::push(uint8_t byte, Mp3Frame& frame) {
    bytes_[size_++] = byte;
    if (size_ < bytes_.size()) return false;
    uint16_t sum = 0;
    for (std::size_t i = 1; i <= 6; ++i) sum += bytes_[i];
    const uint16_t checksum = static_cast<uint16_t>(bytes_[7] << 8) | bytes_[8];
    if (bytes_[0] == 0x7e && bytes_[1] == 0xff && bytes_[2] == 6 &&
        bytes_[4] <= 1 && bytes_[9] == 0xef && static_cast<uint16_t>(0 - sum) == checksum) {
        frame = {bytes_[3], static_cast<uint16_t>((bytes_[5] << 8) | bytes_[6])};
        size_ = 0;
        return true;
    }
    // Sliding window recovers even when a corrupt frame contains another header.
    for (std::size_t i = 1; i < bytes_.size(); ++i) bytes_[i - 1] = bytes_[i];
    --size_;
    return false;
}

void YX5200AudioPlayer::fail(const char* reason) {
    status_ = AudioStatus::Failed;
    count_ = 0;
    awaitingStatus_ = false;
    errorEvent_ = true;
    log_.log(reason);
}

bool YX5200AudioPlayer::takeError() {
    const bool event = errorEvent_;
    errorEvent_ = false;
    return event;
}

bool YX5200AudioPlayer::begin() {
    head_ = count_ = 0;
    initStep_ = 0;
    awaitingStatus_ = false;
    errorEvent_ = false;
    lastTrack_ = 0;
    uartStarted_ = false;
    parser_.reset();
    startedAt_ = lastSent_ = lastByte_ = clock_.now();
    status_ = AudioStatus::Starting;
    if (!uart_.begin()) {
        fail("[ERROR] YX5200 UART initialization failed");
        return false;
    }
    uartStarted_ = true;
    log_.log("[AUDIO] YX5200 initialization pending");
    return true;
}

bool YX5200AudioPlayer::send(uint8_t command, uint16_t parameter) {
    const auto bytes = encodeMp3Frame(command, parameter);
    if (!uart_.write(bytes.data(), bytes.size())) {
        fail("[ERROR] YX5200 UART write failed");
        return false;
    }
    lastSent_ = clock_.now();
    if (command == 0x12) lastTrack_ = parameter;
    if (command == 0x16) lastTrack_ = 0;
    return true;
}

bool YX5200AudioPlayer::enqueue(uint8_t command, uint16_t parameter) {
    if (status_ != AudioStatus::Ready || count_ == queue_.size()) return false;
    queue_[(head_ + count_) % queue_.size()] = {command, parameter};
    ++count_;
    return true;
}

bool YX5200AudioPlayer::playTrack(uint16_t track) {
    return track >= 1 && track <= Config::MaxTrack && enqueue(0x12, track);
}
bool YX5200AudioPlayer::stop() {
    count_ = 0;
    awaitingStatus_ = false;
    if (!uartStarted_ || (status_ != AudioStatus::Ready && status_ != AudioStatus::Failed)) return false;
    const bool retry = status_ == AudioStatus::Ready;
    if (!send(0x16)) return false;
    return !retry || enqueue(0x16);
}
bool YX5200AudioPlayer::pause() { return enqueue(0x0e); }
bool YX5200AudioPlayer::resume() { return enqueue(0x0d); }
bool YX5200AudioPlayer::next() { return enqueue(0x01); }
bool YX5200AudioPlayer::previous() { return enqueue(0x02); }
bool YX5200AudioPlayer::setVolume(int volume) {
    return volume >= 0 && volume <= Config::MaxVolume && enqueue(0x06, static_cast<uint16_t>(volume));
}

void YX5200AudioPlayer::receive(const Mp3Frame& frame) {
    if (frame.command == 0x40) {
        char message[64];
        std::snprintf(message, sizeof(message), "[ERROR] YX5200 module error %u", frame.parameter);
        if (status_ == AudioStatus::Ready && (frame.parameter == 5 || frame.parameter == 6)) {
            count_ = 0;
            errorEvent_ = lastTrack_ != Config::ErrorTrack;
            log_.log(message);
        } else fail(message);
    } else if (frame.command == 0x3b && (frame.parameter & 2)) {
        fail("[ERROR] YX5200 SD card removed");
    } else if (status_ == AudioStatus::Starting && initStep_ == Initialization.size() &&
               frame.command == 0x43 && frame.parameter == Config::DefaultVolume) {
        status_ = AudioStatus::Ready;
        queriedAt_ = clock_.now();
        log_.log("[OK] YX5200 (UART and volume verified)");
    } else if (status_ == AudioStatus::Ready && frame.command == 0x42 && awaitingStatus_) {
        awaitingStatus_ = false;
        queriedAt_ = clock_.now();
    } else if (status_ == AudioStatus::Ready && frame.command == 0x3d) {
        log_.log("[AUDIO] track finished");
    }
}

void YX5200AudioPlayer::update() {
    if (status_ == AudioStatus::Off || status_ == AudioStatus::Failed) return;
    const auto now = clock_.now();
    if (elapsed(now, lastByte_, Config::FrameTimeoutMs)) parser_.reset();
    for (std::size_t i = 0; i < Config::IoBudget; ++i) {
        const int byte = uart_.read();
        if (byte < 0) break;
        lastByte_ = now;
        Mp3Frame frame{};
        if (parser_.push(static_cast<uint8_t>(byte), frame)) receive(frame);
        if (status_ == AudioStatus::Failed) return;
    }
    if (status_ == AudioStatus::Starting) {
        if (!elapsed(now, startedAt_, Config::AudioBootMs)) return;
        if (initStep_ < Initialization.size() && elapsed(now, lastSent_, Config::AudioCommandMs)) {
            if (send(Initialization[initStep_].command, Initialization[initStep_].parameter)) ++initStep_;
        } else if (initStep_ == Initialization.size() && elapsed(now, lastSent_, Config::AudioResponseMs)) {
            fail("[ERROR] YX5200 initialization failed (volume response timeout)");
        }
        return;
    }
    if (awaitingStatus_ && elapsed(now, queriedAt_, Config::AudioResponseMs)) {
        fail("[ERROR] YX5200 disconnected (status response timeout)");
        return;
    }
    if (!elapsed(now, lastSent_, Config::AudioCommandMs)) return;
    const bool stopping = count_ && queue_[head_].command == 0x16;
    if (!stopping && !awaitingStatus_ && elapsed(now, queriedAt_, Config::AudioPollMs)) {
        if (send(0x42)) {
            awaitingStatus_ = true;
            queriedAt_ = now;
        }
    } else if (count_) {
        const auto frame = queue_[head_];
        head_ = (head_ + 1) % queue_.size();
        --count_;
        send(frame.command, frame.parameter);
    }
}

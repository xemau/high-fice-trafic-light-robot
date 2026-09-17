#include "core/YX5200AudioPlayer.h"
#include "core/Timing.h"
#include <cstdio>

namespace {
const char* moduleErrorName(uint16_t code) {
    switch (code) {
        case 1: return "busy/card unavailable";
        case 2: return "sleeping";
        case 3: return "serial frame error";
        case 4: return "checksum mismatch";
        case 5: return "file index out of range";
        case 6: return "file not found/mismatch";
        case 7: return "advertisement error";
        default: return "unknown module error";
    }
}
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
    ++discarded_;
    return false;
}

void YX5200AudioPlayer::trace(const char* event, uint8_t command, uint16_t parameter, const char* detail) {
    char message[192];
    std::snprintf(message, sizeof(message),
                  "[YX5200 %s] t=%lu cmd=0x%02X param=%u audio=%s queue=%u last_named_track=%u %s",
                  event, static_cast<unsigned long>(clock_.now()), static_cast<unsigned>(command),
                  static_cast<unsigned>(parameter), audioStatusName(status_), static_cast<unsigned>(count_),
                  static_cast<unsigned>(lastTrack_), detail);
    log_.log(message);
}

void YX5200AudioPlayer::reportDiagnostics() {
    char message[320];
    std::snprintf(message, sizeof(message),
                  "[YX5200 STATUS] audio=%s init_step=%u/5 queue=%u last_named_track=%u tx_frames=%lu rx_bytes=%lu rx_frames=%lu discarded_bytes=%lu partial_timeouts=%lu awaiting_status=%u",
                  audioStatusName(status_), static_cast<unsigned>(initStep_), static_cast<unsigned>(count_),
                  static_cast<unsigned>(lastTrack_), static_cast<unsigned long>(txFrames_),
                  static_cast<unsigned long>(rxBytes_), static_cast<unsigned long>(rxFrames_),
                  static_cast<unsigned long>(parser_.discardedBytes()), static_cast<unsigned long>(partialTimeouts_),
                  static_cast<unsigned>(awaitingStatus_));
    log_.log(message);
    std::snprintf(message, sizeof(message),
                  "[YX5200 STATUS] last_tx=0x%02X/%u last_rx=0x%02X/%u last_error=%s",
                  static_cast<unsigned>(lastTx_.command), static_cast<unsigned>(lastTx_.parameter),
                  static_cast<unsigned>(lastRx_.command), static_cast<unsigned>(lastRx_.parameter), lastError_);
    log_.log(message);
}

void YX5200AudioPlayer::fail(const char* reason) {
    std::snprintf(lastError_, sizeof(lastError_), "%s", reason);
    trace("FAIL", lastTx_.command, lastTx_.parameter, "dropping queued commands");
    status_ = AudioStatus::Failed;
    count_ = 0;
    awaitingStatus_ = false;
    errorEvent_ = true;
    log_.log(reason);
    reportDiagnostics();
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
    parser_ = Mp3Parser{};
    rxBytes_ = rxFrames_ = txFrames_ = partialTimeouts_ = 0;
    warnedDiscarded_ = warnedTimeouts_ = 0;
    lastTx_ = lastRx_ = {};
    std::snprintf(lastError_, sizeof(lastError_), "none");
    startedAt_ = lastSent_ = lastByte_ = clock_.now();
    lastRxWarning_ = startedAt_;
    status_ = AudioStatus::Starting;
    if (!uart_.begin()) {
        fail("[ERROR] YX5200 UART initialization failed");
        return false;
    }
    log_.log("[AUDIO] YX5200 initialization pending");
    char message[160];
    std::snprintf(message, sizeof(message),
                  "[YX5200 INIT] baud=%lu rx_gpio=%u tx_gpio=%u settle_ms=%lu response_ms=%lu volume=%d",
                  static_cast<unsigned long>(Config::Mp3Baud), static_cast<unsigned>(Config::Pins::Mp3Rx),
                  static_cast<unsigned>(Config::Pins::Mp3Tx), static_cast<unsigned long>(Config::AudioBootMs),
                  static_cast<unsigned long>(Config::AudioResponseMs), Config::DefaultVolume);
    log_.log(message);
    return true;
}

bool YX5200AudioPlayer::send(uint8_t command, uint16_t parameter) {
    const auto bytes = encodeMp3Frame(command, parameter);
    if (!uart_.write(bytes.data(), bytes.size())) {
        trace("TX_REJECTED", command, parameter, "UART could not write complete frame");
        fail("[ERROR] YX5200 UART write failed");
        return false;
    }
    lastSent_ = clock_.now();
    lastTx_ = {command, parameter};
    ++txFrames_;
    if (command == 0x12) lastTrack_ = parameter;
    if (command == 0x16) lastTrack_ = 0;
    if (command == 0x01 || command == 0x02) lastTrack_ = 0;
    trace("TX", command, parameter, "written to UART; not proof of playback");
    return true;
}

bool YX5200AudioPlayer::enqueue(uint8_t command, uint16_t parameter) {
    if (status_ != AudioStatus::Ready) {
        trace("REJECT", command, parameter, "audio not ready");
        return false;
    }
    if (count_ == queue_.size()) {
        trace("REJECT", command, parameter, "command queue full");
        return false;
    }
    queue_[(head_ + count_) % queue_.size()] = {command, parameter};
    ++count_;
    trace("QUEUED", command, parameter, "not yet sent");
    return true;
}

bool YX5200AudioPlayer::playTrack(uint16_t track) {
    if (track < 1 || track > Config::MaxTrack) {
        trace("REJECT", 0x12, track, "track outside 1..9999");
        return false;
    }
    char message[96];
    std::snprintf(message, sizeof(message), "[AUDIO PLAY] track=%u file=/MP3/%04u.mp3",
                  static_cast<unsigned>(track), static_cast<unsigned>(track));
    log_.log(message);
    return enqueue(0x12, track);
}
bool YX5200AudioPlayer::stop() {
    // Stop supersedes pending playback so an expired reward cannot start later.
    if (count_) trace("CANCEL", 0x16, 0, "stop discards queued commands");
    count_ = 0;
    return enqueue(0x16);
}
bool YX5200AudioPlayer::pause() { return enqueue(0x0e); }
bool YX5200AudioPlayer::resume() { return enqueue(0x0d); }
bool YX5200AudioPlayer::next() { return enqueue(0x01); }
bool YX5200AudioPlayer::previous() { return enqueue(0x02); }
bool YX5200AudioPlayer::setVolume(int volume) {
    if (volume < 0 || volume > Config::MaxVolume) {
        char message[80];
        std::snprintf(message, sizeof(message), "[ERROR] volume=%d outside 0..30; command not sent", volume);
        log_.log(message);
        return false;
    }
    return enqueue(0x06, static_cast<uint16_t>(volume));
}

void YX5200AudioPlayer::receive(const Mp3Frame& frame) {
    ++rxFrames_;
    lastRx_ = frame;
    trace("RX", frame.command, frame.parameter, "valid checksummed frame");
    if (frame.command == 0x40) {
        char message[192];
        std::snprintf(message, sizeof(message), "[ERROR] YX5200 module error %u (%s); last_named_track=%u last_tx=0x%02X/%u",
                      static_cast<unsigned>(frame.parameter), moduleErrorName(frame.parameter),
                      static_cast<unsigned>(lastTrack_), static_cast<unsigned>(lastTx_.command),
                      static_cast<unsigned>(lastTx_.parameter));
        if (status_ == AudioStatus::Ready && (frame.parameter == 5 || frame.parameter == 6)) {
            std::snprintf(lastError_, sizeof(lastError_), "%s", message);
            count_ = 0;
            errorEvent_ = lastTrack_ != Config::ErrorTrack;
            log_.log(message);
            log_.log(errorEvent_ ? "[YX5200 ERROR] error cue event raised; queued commands discarded" :
                      "[SOUND] role=error result=suppressed reason=error track failed; no recursive error cue");
        } else fail(message);
    } else if (frame.command == 0x3b && (frame.parameter & 2)) {
        fail("[ERROR] YX5200 SD card removed");
    } else if (status_ == AudioStatus::Starting && initStep_ == 5 &&
               frame.command == 0x43 && frame.parameter == Config::DefaultVolume) {
        status_ = AudioStatus::Ready;
        queriedAt_ = clock_.now();
        log_.log("[OK] YX5200 (UART and volume verified)");
    } else if (status_ == AudioStatus::Ready && frame.command == 0x42 && awaitingStatus_) {
        awaitingStatus_ = false;
        queriedAt_ = clock_.now();
    } else if (status_ == AudioStatus::Ready && frame.command == 0x3d) {
        char message[160];
        std::snprintf(message, sizeof(message),
                      "[AUDIO] track finished reported_index=%u last_named_track=%u (index may differ from filename ID)",
                      static_cast<unsigned>(frame.parameter), static_cast<unsigned>(lastTrack_));
        log_.log(message);
    } else if (status_ == AudioStatus::Starting && frame.command == 0x43) {
        char message[112];
        std::snprintf(message, sizeof(message), "[WARN] volume reply not accepted: got=%u expected=%d init_step=%u/5",
                      static_cast<unsigned>(frame.parameter), Config::DefaultVolume, static_cast<unsigned>(initStep_));
        log_.log(message);
    }
}

void YX5200AudioPlayer::update() {
    if (status_ == AudioStatus::Off || status_ == AudioStatus::Failed) return;
    const auto now = clock_.now();
    if (parser_.pendingBytes() && elapsed(now, lastByte_, Config::FrameTimeoutMs)) {
        ++partialTimeouts_;
        parser_.reset();
    }
    for (std::size_t i = 0; i < Config::IoBudget; ++i) {
        const int byte = uart_.read();
        if (byte < 0) break;
        ++rxBytes_;
        lastByte_ = now;
        Mp3Frame frame{};
        if (parser_.push(static_cast<uint8_t>(byte), frame)) receive(frame);
        if (status_ == AudioStatus::Failed) return;
    }
    if ((parser_.discardedBytes() != warnedDiscarded_ || partialTimeouts_ != warnedTimeouts_) &&
        elapsed(now, lastRxWarning_, 1000)) {
        warnedDiscarded_ = parser_.discardedBytes();
        warnedTimeouts_ = partialTimeouts_;
        lastRxWarning_ = now;
        log_.log("[WARN] YX5200 malformed/incomplete RX data; check baud, UART wiring, ground and power");
        reportDiagnostics();
    }
    if (status_ == AudioStatus::Starting) {
        if (!elapsed(now, startedAt_, Config::AudioBootMs)) return;
        if (initStep_ < 5 && elapsed(now, lastSent_, Config::AudioCommandMs)) {
            const Mp3Frame init[] = {{0x16, 0}, {0x09, 2}, {0x06, Config::DefaultVolume}, {0x1a, 0}, {0x43, 0}};
            if (send(init[initStep_].command, init[initStep_].parameter)) ++initStep_;
        } else if (initStep_ == 5 && elapsed(now, lastSent_, Config::AudioResponseMs)) {
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

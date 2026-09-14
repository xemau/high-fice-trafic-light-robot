#include "core/CommandParser.h"
#include <cstring>

namespace {
bool space(char c) { return c == ' ' || c == '\t'; }
bool equal(const char* a, const char* b) { return std::strcmp(a, b) == 0; }
}

Command parseCommand(const char* line) {
    if (!line) return {};
    char text[Config::SerialLineSize]{};
    std::size_t n = 0;
    for (; line[n]; ++n) {
        if (n >= sizeof(text) - 1) return {};
        const char c = line[n];
        if (c != '\t' && (c < ' ' || c > '~')) return {};
        text[n] = c;
    }
    while (n && space(text[n - 1])) text[--n] = 0;
    char* first = text;
    while (space(*first)) ++first;
    char* arg = first;
    while (*arg && !space(*arg)) ++arg;
    if (*arg) *arg++ = 0;
    while (space(*arg)) ++arg;

    if (equal(first, "lights")) {
        if (equal(arg, "red")) return {CommandType::Red};
        if (equal(arg, "yellow")) return {CommandType::Yellow};
        if (equal(arg, "green")) return {CommandType::Green};
        if (equal(arg, "off")) return {CommandType::Off};
        return {};
    }
    if (equal(first, "simulate") && equal(arg, "highfive")) return {CommandType::HighFive};
    if (equal(first, "play") || equal(first, "volume")) {
        if (!*arg) return {};
        int value = 0;
        for (const char* p = arg; *p; ++p) {
            if (*p < '0' || *p > '9') return {};
            value = value * 10 + (*p - '0');
            if (value > Config::MaxTrack) return {};
        }
        if (equal(first, "play") && value >= 1) return {CommandType::Play, value};
        if (equal(first, "volume") && value <= Config::MaxVolume) return {CommandType::Volume, value};
        return {};
    }
    if (*arg) return {};
    if (first[0] >= '1' && first[0] <= '5' && !first[1]) return {CommandType::SelectMode, first[0] - '0'};
    struct Word { const char* text; CommandType type; };
    constexpr Word words[] = {
        {"red", CommandType::Red}, {"yellow", CommandType::Yellow}, {"green", CommandType::Green},
        {"off", CommandType::Off}, {"cycle", CommandType::Cycle}, {"dance", CommandType::Dance},
        {"stop", CommandType::Stop}, {"pause", CommandType::Pause}, {"resume", CommandType::Resume},
        {"next", CommandType::Next}, {"previous", CommandType::Previous}, {"highfive", CommandType::HighFive},
        {"status", CommandType::Status}, {"reset", CommandType::Reset}, {"help", CommandType::Help},
        {"retry", CommandType::Retry}
    };
    for (const auto& word : words) if (equal(first, word.text)) return {word.type};
    return {};
}

LineResult LineBuffer::push(char c) {
    if (c == '\r' || c == '\n') {
        const bool wasRejected = rejected_;
        const bool empty = size_ == 0;
        buffer_[size_] = 0;
        size_ = 0;
        rejected_ = false;
        if (wasRejected) return LineResult::Rejected;
        return empty ? LineResult::Pending : LineResult::Complete;
    }
    if (rejected_) return LineResult::Pending;
    if (c == '\b' || c == 127) {
        if (size_) --size_;
    } else if (size_ == sizeof(buffer_) - 1 || (c != '\t' && (c < ' ' || c > '~'))) {
        rejected_ = true;
    } else {
        buffer_[size_++] = c;
    }
    return LineResult::Pending;
}

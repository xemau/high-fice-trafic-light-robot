#pragma once
#include "Config.h"

enum class CommandType {
    Invalid, SelectMode, Red, Yellow, Green, Off, Cycle, Dance,
    Play, Stop, Pause, Resume, Volume, Next, Previous, HighFive, Status, Reset, Help, Retry, BootSound, ErrorSound
};
struct Command { CommandType type = CommandType::Invalid; int value = 0; };
Command parseCommand(const char* line);
enum class LineResult { Pending, Complete, Rejected };
class LineBuffer {
public:
    LineResult push(char c);
    const char* text() const { return buffer_; }
private:
    char buffer_[Config::SerialLineSize]{};
    std::size_t size_ = 0;
    bool rejected_ = false;
};

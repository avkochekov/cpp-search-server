#pragma once

//=================================================================================
#include <chrono>
#include <iostream>

//=================================================================================
#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)              /// ALARMA!!!

//=================================================================================
class LogDuration {
public:
    // заменим имя типа std::chrono::steady_clock
    // с помощью using для удобства
    using Clock = std::chrono::steady_clock;

    LogDuration(const std::string_view& id, std::ostream& stream = std::cerr);

    ~LogDuration();

private:
    std::ostream& stream;
    const std::string id_;
    const Clock::time_point start_time_ = Clock::now();
};

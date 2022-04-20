#include "log_duration.h"

//=================================================================================
LogDuration::LogDuration(const std::string_view &id, std::ostream &stream) : stream(stream), id_(id) {}

//=================================================================================
LogDuration::~LogDuration() {
    using namespace std::chrono;
    using namespace std::literals;

    const auto end_time = Clock::now();
    const auto dur = end_time - start_time_;
    stream << "Operation time: "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
}

#ifndef _CLOCK_H_
#define _CLOCK_H_

#include <mutex>

namespace servx {

class Clock {
public:
    Clock(const Clock&) = delete;
    Clock(Clock&&) = delete;
    Clock& operator=(const Clock&) = delete;
    Clock& operator=(Clock&&) = delete;

    ~Clock() = default;

    void update();

    uint64_t get_current_milliseconds() const { return milliseconds[cur]; }

    const std::string& get_current_http_time() const { return http_time[cur]; }

    const std::string& get_current_log_time() const { return log_time[cur]; }

    static Clock* instance() { return clock; }

    static int format_http_time(time_t sec, char* buf);

private:
    Clock();

    std::mutex mtx;

    std::string http_time[2];
    std::string log_time[2];
    uint64_t milliseconds[2];

    uint8_t cur;

    static const char* week[];
    static const char* months[];

    static Clock* clock;
};

}

#endif

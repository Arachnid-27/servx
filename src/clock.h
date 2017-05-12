#ifndef _CLOCK_H_
#define _CLOCK_H_

#include <mutex>

namespace servx {

class Clock {
public:
    Clock(const Clock&) = delete;

    void update();

    time_t get_current_ms() const { return current_ms; }

public:
    static Clock* instance() { return clock; }

private:
    Clock() = default;

private:
    std::mutex mtx;
    time_t current_ms;

private:
    static Clock* clock;
};

}

#endif

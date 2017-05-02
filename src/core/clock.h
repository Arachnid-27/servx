#ifndef _CLOCK_H_
#define _CLOCK_H_

#include "core.h"

class Clock {
public:
    void update();

    std::time_t get_current_ms() const { return current_ms; }

    static Clock* instance() { return clock; }

private:
    Clock() = default;
    
    Clock(const Clock&) = delete;

    std::time_t get_milliseconds();

private:
    std::mutex mtx;
    std::time_t current_ms;

private:
    static Clock* clock;
};

#endif

#include "clock.h"

Clock* Clock::clock = new Clock;

void Clock::update() {
    if (!mtx.try_lock()) {
        return;
    }

    current_ms  = get_milliseconds();

    mtx.unlock();
}

inline std::time_t Clock::get_milliseconds() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

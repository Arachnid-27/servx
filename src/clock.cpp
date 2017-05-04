#include "clock.h"

#include <chrono>

namespace servx {

Clock* Clock::clock = new Clock;

void Clock::update() {
    using namespace std::chrono;

    if (!mtx.try_lock()) {
        return;
    }

    auto now = high_resolution_clock::now().time_since_epoch();
    current_ms = duration_cast<milliseconds>(now).count();

    mtx.unlock();
}

}

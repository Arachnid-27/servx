#include "timer.h"

#include "clock.h"

namespace servx {

Timer* Timer::timer = new Timer;

void Timer::add_timer(Event* ev, std::time_t delay) {
    auto now = Clock::instance()->get_current_ms();
    auto timer = now + delay;

    if (ev->is_timer()) {
        if (std::abs(ev->get_timer() - timer) < 100) {
            // err_log
            return;
        }

        del_timer(ev);
    }

    timer_tree.insert(timer_tree.cend(), ev);

    ev->set_timer(timer);
}

void Timer::del_timer(Event* ev) {
    // assume ev->is_timer() is true

    auto it = timer_tree.find(ev);

    if (it != timer_tree.end()) {
        timer_tree.erase(it);
        ev->set_timer(0);
    } else {
        // err_log
    }
}

void Timer::expire_timer() {
    auto now = Clock::instance()->get_current_ms();

    decltype(timer_tree.begin()) it;
    Event *ev;

    while (!timer_tree.empty()) {
        it = timer_tree.begin();
        ev = *it;

        if (ev->get_timer() > now) {
            return;
        }

        timer_tree.erase(it);

        ev->expire();
    }
}

}

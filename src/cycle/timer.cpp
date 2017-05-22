#include "timer.h"

#include "clock.h"
#include "logger.h"

namespace servx {

Timer* Timer::timer = new Timer;

void Timer::add_timer(Event* ev, uint32_t delay) {
    uint64_t now = Clock::instance()->get_current_milliseconds();
    uint64_t timer = now + delay;

    if (ev->is_timer()) {
        if (std::abs(static_cast<int>(ev->get_timer() - timer)) < 300) {
            Logger::instance()->info("timer exist, and expire less than 300ms");
            return;
        }

        del_timer(ev);
    }

    timer_tree.insert(timer_tree.cend(), ev);

    ev->set_timer(timer);
}

void Timer::del_timer(Event* ev) {
    auto it = timer_tree.find(ev);

    if (it != timer_tree.end()) {
        timer_tree.erase(it);
        ev->set_timer(0);
    } else {
        Logger::instance()->warn("can not find timer");
    }
}

void Timer::expire_timer() {
    auto now = Clock::instance()->get_current_milliseconds();

    decltype(timer_tree.begin()) it;
    Event *ev;

    while (!timer_tree.empty()) {
        it = timer_tree.begin();
        ev = *it;

        if (ev->get_timer() > now) {
            return;
        }

        timer_tree.erase(it);

        Logger::instance()->debug("timer expired");

        ev->expire();
    }
}

}

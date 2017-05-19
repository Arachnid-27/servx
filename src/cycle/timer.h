#ifndef _TIMER_H_
#define _TIMER_H_

#include <set>

#include "connection.h"

namespace servx {

class TimerCompare {
public:
    bool operator()(Event* lhs, Event* rhs) const {
        return lhs->get_timer() < rhs->get_timer();
    }
};

class Timer {
public:
    Timer(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer& operator=(Timer&&) = delete;

    ~Timer() = default;

    void add_timer(Event* ev, uint32_t delay);

    void del_timer(Event* ev);

    void expire_timer();

public:
    static Timer* instance() { return timer; }

private:
    Timer() = default;

private:
    std::set<Event*, TimerCompare> timer_tree;

private:
    static Timer *timer;
};

}

#endif

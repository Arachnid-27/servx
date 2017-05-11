#ifndef _EVENT_H_
#define _EVENT_H_

#include <ctime>
#include <functional>
#include <memory>

#include "connection.h"

namespace servx {

class Connection;

class Event {
public:
    Event(Connection* c, bool w)
        : conn(c), write(w), active(0),
          ready(0), timeout(0), timer(0) {}

    Connection* get_connection() const { return conn; }

    bool is_write_event() const { return write == 1; }

    bool is_active() const { return active == 1; }

    void set_active(bool a) { active = a; }

    bool is_timer() const { return timer != 0; }

    time_t get_timer() const { return timer; }

    void set_timer(time_t t) { timer = t; }

    bool is_ready() const { return ready; }

    void set_ready(bool r) { ready = r; }

    bool is_timeout() const { return timeout; }

    void expire() {
        timer = 0;
        timeout = 1;
        handler(this);
        timeout = 0;
    }

    void handle() { handler(this); }

private:
    Connection* conn;
    uint32_t write:1;
    uint32_t active:1;
    uint32_t ready:1;
    uint32_t timeout:1;
    time_t timer;
    std::function<void(Event*)> handler;
};

}

#endif

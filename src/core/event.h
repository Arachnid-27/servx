#ifndef _EVENT_H_
#define _EVENT_H_

#include <functional>

#include "connection.h"

namespace servx {

class Connection;

class Event {
public:
    Event(Connection* c, bool w)
        : connection(c), write(w), active(0),
          ready(0), timeout(0), timer(0) {}

    Connection* get_connection() const { return connection; }

    bool is_write_event() const { return write == 1; }

    bool is_active() const { return active == 1; }

    void set_active(bool a) { active = a; }

    bool is_timer() const { return timer != 0; }

    std::time_t get_timer() const { return timer; }

    void set_timer(std::time_t t) { timer = t; }

    bool is_ready() const { return ready; }

    void set_ready(bool r) { ready = r; }

    bool is_timeout() const { return timeout; }

    void expire() {
        timer = 0;
        timeout = 1;
        handler(this);
    }

    void handle() { handler(this); }

private:
    Connection *connection;
    unsigned int write:1;
    unsigned int active:1;
    unsigned int ready:1;
    unsigned int timeout:1;
    std::time_t timer;
    std::function<void(Event*)> handler;
};

}

#endif

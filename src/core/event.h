#ifndef _EVENT_H_
#define _EVENT_H_


#include "connection.h"

#include <functional>

class Connection;


class Event {
public:
    Event(Connection* c, bool w)
        : connection(c), write(w), event_active(0), timer_active(0), 
          ready(0), timeout(0) {}

    Connection* get_connection() const { return connection; }

    bool is_write_event() const { return write == 1; }

    bool is_event_active() const { return event_active == 1; }

    void set_event_active(bool a) { event_active = a; }

    bool is_timer_active() const { return timer_active == 1; }

    void set_timer_active(bool a) { timer_active = a; }

    bool is_ready() const { return ready; }

    void set_ready(bool r) { ready = r; }

    void handle() { handler(this); }

private:
    Connection *connection;
    unsigned int write:1;
    unsigned int event_active:1;
    unsigned int timer_active:1;
    unsigned int ready:1;
    unsigned int timeout:1;
    std::function<void(Event*)> handler;
};


#endif

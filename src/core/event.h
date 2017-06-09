#ifndef _EVENT_H_
#define _EVENT_H_

#include <functional>

namespace servx {

class Connection;

class Event {
    using event_handler_t = std::function<void(Event*)>;
public:
    Event(Connection* c, bool w): conn(c), write(w) { reset(); }

    Event(const Event&) = delete;
    Event(Event&&) = delete;
    Event& operator=(const Event&) = delete;
    Event& operator=(Event&&) = delete;

    ~Event() = default;

    Connection* get_connection() const { return conn; }

    bool is_write_event() const { return write == 1; }

    bool is_active() const { return active == 1; }
    void set_active(bool a) { active = a; }

    uint64_t get_timer() const { return timer; }
    bool is_timer() const { return timer != 0; }
    void set_timer(uint64_t t) { timer = t; }

    bool is_ready() const { return ready; }
    void set_ready(bool r) { ready = r; }

    bool is_eof() const { return eof; }
    void set_eof(bool e) { eof = e; }

    bool is_timeout() const { return timeout; }

    void handle() { handler(this); }
    void set_handler(const event_handler_t& h) { handler = h; }

    void expire() {
        timer = 0;
        timeout = 1;
        handler(this);
    }

    void reset() {
        active = ready = timeout = eof = 0;
        timer = 0;
        handler = write ? empty_write_handler : empty_read_handler;
    }

    static void empty_read_handler(Event*) {}

    static void empty_write_handler(Event*) {}

private:
    Connection *conn;
    uint32_t write:1;
    uint32_t active:1;
    uint32_t ready:1;
    uint32_t timeout:1;
    uint32_t eof:1;
    uint64_t timer;
    event_handler_t handler;
};

}

#endif

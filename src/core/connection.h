#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "clock.h"
#include "event.h"

namespace servx {

class Event;

class Connection {
public:
    Connection();

    Connection(const Connection&) = delete;

    int get_fd() const { return fd; }

    Event* get_read_event() const { return read_event; }

    Event* get_write_event() const { return write_event; }

private:
    int fd;
    Event* read_event;
    Event* write_event;
};

}

#endif

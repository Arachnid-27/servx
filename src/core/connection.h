#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "event.h"

namespace servx {

class Event;

class Connection {
public:
    Connection();

    Connection(const Connection&) = delete;

    void open(int fd);

    void close();

    bool is_close() const { return socket_fd == -1; }

    int get_fd() const { return socket_fd; }

    Event* get_read_event() { return read_event; }

    Event* get_write_event() { return write_event; }

private:
    int socket_fd;
    Event *read_event;
    Event *write_event;
};

}

#endif

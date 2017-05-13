#include "connection.h"

#include <unistd.h>

#include <cstring>

namespace servx {

Event::Event(Connection* c, bool w)
    : conn(c), write(w) {
    reset();
}

void Event::expire() {
    timer = 0;
    timeout = 1;
    handler(this);
    timeout = 0;
}

void Event::reset() {
    active = 0;
    ready = 0;
    timeout = 0;
    timer = 0;
}

uint64_t Connection::count = 0;

Connection::Connection()
    : socket_fd(-1), read_event(this, false),
      write_event(this, true) {
}

bool Connection::open(int fd, bool lst) {
    socket_fd = fd;
    conn_id = ++count;
    listen = lst;

    sockaddr sa;
    socklen_t len;

    if (getsockname(fd, &sa, &len) == -1) {
        return false;
    }
    local_addr.set_addr(&sa, len);

    return true;
}

void Connection::close() {
    if (::close(socket_fd) == -1) {
        // err_log
    }
    socket_fd = -1;
}

}

#include "connection.h"

#include <unistd.h>

#include <cstring>

namespace servx {

uint64_t Connection::count = 0;

Connection::Connection()
    : socket_fd(-1),
      read_event(new Event(this, false)),
      write_event(new Event(this, true)) {
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

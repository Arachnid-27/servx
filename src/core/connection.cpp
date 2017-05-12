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

void Connection::open(int fd, bool lst) {
    socket_fd = fd;
    conn_id = ++count;
    listen = lst;
}

void Connection::close() {
    if (::close(socket_fd) == -1) {
        // err_log
    }
}

}

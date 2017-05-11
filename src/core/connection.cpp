#include "connection.h"

#include <unistd.h>

#include <cstring>

namespace servx {

Connection::Connection()
    : socket_fd(-1),
      read_event(new Event(this, false)),
      write_event(new Event(this, true)) {
}

void Connection::open(int fd) {
    socket_fd = fd;
}

void Connection::close() {
    if (::close(socket_fd) == -1) {
        // err_log
    }
}

}

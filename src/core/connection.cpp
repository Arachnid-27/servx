#include "connection.h"

#include <unistd.h>

#include <cstring>

#include "connection_pool.h"
#include "core.h"
#include "event_module.h"
#include "io.h"
#include "logger.h"
#include "timer.h"

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
    eof = 0;
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

    char sa[sizeof(sockaddr_in6)];
    socklen_t len = sizeof(sockaddr_in6);

    if (getsockname(fd, reinterpret_cast<sockaddr*>(sa), &len) == -1) {
        Logger::instance()->error("get peer sockaddr failed");
        return false;
    }
    local_addr.set_addr(reinterpret_cast<sockaddr*>(sa), len);

    return true;
}

void Connection::close() {
    if (socket_fd == -1) {
        Logger::instance()->warn("connection already closed");
        return;
    }

    if (read_event.is_timer()) {
        Timer::instance()->del_timer(&read_event);
    }

    if (write_event.is_timer()) {
        Timer::instance()->del_timer(&write_event);
    }

    del_connection(this);

    ctx.release();
    recv_buf.release();

    ConnectionPool::instance()->ret_connection(this);

    if (::close(socket_fd) == -1) {
        Logger::instance()->warn("close %d failed", socket_fd);
    }

    socket_fd = -1;
}

void Connection::init_recv_buf(int sz) {
    recv_buf = std::unique_ptr<Buffer>(new Buffer(sz));
}

int Connection::recv_data() {
    int rc = io_recv(socket_fd, recv_buf.get());

    switch (rc) {
    case SERVX_OK:
    case SERVX_DENY:
        return rc;
    case SERVX_DONE:
        read_event.set_eof(true);
        break;
    case SERVX_ERROR:
        read_event.set_error(true);
        break;
    }

    read_event.set_ready(false);
    return rc;
}

int Connection::send_data(char* data, int size) {
    if (size < 0) {
        return SERVX_ERROR;
    }

    Buffer buf(data, size, false);
    int rc = io_send(socket_fd, &buf);

    // Todo record bytes the conn sent

    switch (rc) {
    case SERVX_OK:
        break;
    case SERVX_DONE:
        read_event.set_eof(true);
        break;
    case SERVX_ERROR:
        read_event.set_error(true);
        break;
    }

    read_event.set_ready(false);

    return rc;
}

}

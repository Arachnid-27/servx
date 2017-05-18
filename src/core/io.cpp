#include "io.h"

#include <errno.h>
#include <unistd.h>

namespace servx {

int recv(Connection* conn) {
    Buffer *buf = conn->get_recv_buf();
    Event *ev = conn->get_read_event();
    int size = buf->get_remain();

    if (size == 0) {
        return IO_BUF_TOO_SMALL;
    }

    // Todo EPOLLRDHUP

    while (true) {
        int n = ::read(conn->get_fd(), buf->get_last(), size);

        if (n > 0) {
            buf->set_last(buf->get_last() + n);

            if (n < size) {
                ev->set_ready(false);
                return IO_PARTIAL;
            }

            return IO_SUCCESS;
        }

        if (n == 0) {
            ev->set_ready(false);
            ev->set_eof(true);
            return IO_FINISH;
        }

        if (errno == EINTR) {
            continue;
        }

        ev->set_ready(false);

        if (errno == EAGAIN) {
            return IO_BLOCK;
        }

        ev->set_error(true);
        return IO_ERROR;
    }
}

int send(Connection* conn, char* buf, uint32_t size) {
    Event *ev = conn->get_write_event();

    while (true) {
        int n = ::write(conn->get_fd(), buf, size);

        if (n >= 0) {
            if (n < static_cast<int>(size)) {
                ev->set_ready(false);
                return IO_PARTIAL;
            }

            // Todo record bytes the conn sent

            return IO_SUCCESS;
        }

        if (errno == EINTR) {
            continue;
        }

        ev->set_ready(false);

        if (errno == EAGAIN) {
            return IO_BLOCK;
        }

        ev->set_error(true);
        return IO_ERROR;
    }
}

}

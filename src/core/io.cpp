#include "io.h"

#include <errno.h>
#include <unistd.h>

namespace servx {

int recv(Connection* conn, Buffer* buf) {
    Event *ev = conn->get_read_event();
    int n, size = buf->get_remain();

    if (size == 0) {
        return IO_BUF_TOO_SMALL;
    }

    // Todo EPOLLRDHUP

    while (true) {
        n = ::read(conn->get_fd(), buf->get_last(), size);

        if (n > 0) {
            if (n < size) {
                ev->set_ready(false);
            }

            buf->set_last(buf->get_last() + n);

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

}

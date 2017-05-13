#include "io.h"

#include <errno.h>
#include <unistd.h>

namespace servx {

int recv(Connection* conn, size_t size) {
    Buffer *buf = conn->get_recv_buf();
    int n;

    // Todo EPOLLRDHUP

    while (true) {
        n = ::read(conn->get_fd(), buf->get_pos(), size);

        if (n > 0) {
            if (static_cast<size_t>(n) < size) {
                conn->get_read_event()->set_ready(false);
            }

            buf->set_last(buf->get_pos() + n);

            return n;
        }

        if (n == 0) {
            conn->get_read_event()->set_ready(false);
            conn->get_read_event()->set_eof(true);
            return 0;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno != EAGAIN) {
            conn->get_read_event()->set_error(true);
        }
        conn->get_read_event()->set_ready(false);

        return n;
    }
}

}

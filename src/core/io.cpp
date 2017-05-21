#include "io.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

#include "core.h"

namespace servx {

int io_recv(int fd, Buffer* buf) {
    int size = buf->get_remain();
    char *pos = buf->get_last();

    // Todo EPOLLRDHUP

    while (true) {
        int n = read(fd, pos, size);

        if (n > 0) {
            buf->set_last(pos + n);
            return n < size ? SERVX_PARTIAL : SERVX_OK;
        }

        if (n == 0) {
            return SERVX_DONE;
        }

        int err = errno;

        if (err == EINTR) {
            continue;
        }

        if (err == EAGAIN) {
            return SERVX_AGAIN;
        }

        return SERVX_ERROR;
    }
}

int io_send(int fd, Buffer* buf) {
    int size = buf->get_size();
    char *pos = buf->get_pos();

    while (true) {
        int n = write(fd, pos, size);

        if (n >= 0) {
            buf->set_pos(pos + n);
            return n < size ? SERVX_PARTIAL : SERVX_OK;
        }

        int err = errno;

        if (err == EINTR) {
            continue;
        }

        if (err == EAGAIN) {
            return SERVX_AGAIN;
        }

        return SERVX_ERROR;
    }
}

int io_send_chain(int fd, std::list<Buffer>& chain) {
    struct iovec iovs[64];
    int total = 0;
    int cnt = 0;

    for (auto &buf : chain) {
        iovs[cnt].iov_base = static_cast<void*>(buf.get_pos());
        iovs[cnt].iov_len = buf.get_size();
        total += buf.get_size();
        if (++cnt == 64) {
            break;
        }
    }

    while (true) {
        int n = writev(fd, iovs, cnt);

        if (n >= 0) {
            if (n < total) {
                int size;
                for (auto &buf : chain) {
                    size = buf.get_size();
                    if (n <= size) {
                        buf.set_pos(buf.get_pos() + n);
                        break;
                    } else {
                        buf.set_pos(buf.get_last());
                        n -= size;
                    }
                }
                return SERVX_PARTIAL;
            } else {
                for (auto &buf : chain) {
                    if (cnt == 0) {
                        return SERVX_PARTIAL;
                    }
                    buf.set_pos(buf.get_last());
                    --cnt;
                }
                return SERVX_OK;
            }
        }

        int err = errno;

        if (err == EINTR) {
            continue;
        }

        if (err == EAGAIN) {
            return SERVX_AGAIN;
        }

        return SERVX_ERROR;
    }
}

}

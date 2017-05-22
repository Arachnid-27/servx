#include "io.h"

#include <errno.h>
#include <sys/sendfile.h>
#include <unistd.h>

#include "core.h"

namespace servx {

int io_read(int fd, char* buf, uint32_t count) {
    while (true) {
        int n = read(fd, buf, count);

        if (n >= 0) {
            return n;
        }

        int err = errno;

        switch (err) {
        case EINTR:
            break;
        case EAGAIN:
            return SERVX_AGAIN;
        default:
            return SERVX_ERROR;
        }
    }
}

int io_write(int fd, char* buf, uint32_t count) {
    while (true) {
        int n = write(fd, buf, count);

        if (n >= 0) {
            return n;
        }

        int err = errno;

        switch (err) {
        case EINTR:
            break;
        case EAGAIN:
            return SERVX_AGAIN;
        default:
            return SERVX_ERROR;
        }
    }
}

int io_write_chain(int fd, struct iovec* iov, uint32_t count) {
    while (true) {
        int n = writev(fd, iov, count);

        if (n >= 0) {
            return n;
        }

        int err = errno;

        switch (err) {
        case EINTR:
            break;
        case EAGAIN:
            return SERVX_AGAIN;
        default:
            return SERVX_ERROR;
        }
    }
}

int io_sendfile(int out_fd, int in_fd, long* offset, uint32_t count) {
    while (true) {
        int n = sendfile(out_fd, in_fd, offset, count);

        if (n > 0) {
            return n;
        }

        if (n == 0) {
            // someone has truncated the file
            return SERVX_ERROR;
        }

        int err = errno;

        switch (err) {
        case EINTR:
            break;
        case EAGAIN:
            return SERVX_AGAIN;
        default:
            return SERVX_ERROR;
        }
    }
}

}

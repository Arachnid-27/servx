#include "file.h"

#include <errno.h>
#include <sys/sendfile.h>

#include "core.h"

namespace servx {

File::~File() {
    if (fd != -1) {
        ::close(fd);
    }
}

bool File::file_status() {
    if (info != nullptr) {
        return true;
    }

    if (fd == -1) {
        return false;
    }

    struct stat *st = new struct stat;
    if (fstat(fd, st) == -1) {
        return false;
    }

    info = std::unique_ptr<struct stat>(st);
    return true;
}

int File::send(int out_fd, int count) {
    while (true) {
        int n = sendfile(out_fd, fd, &offset, count);

        if (n > 0) {
            return n < count ? SERVX_PARTIAL : SERVX_OK;
        }

        if (n == 0) {
            // someone has truncated the file
            return SERVX_ERROR;
        }

        int err = errno;

        if (err == EINTR) {
            continue;
        }

        if (err == EAGAIN) {
            return SERVX_NOT_READY;
        }

        return SERVX_ERROR;
    }
}

}

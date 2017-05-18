#include "file.h"

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

}

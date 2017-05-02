#include "daemon.h"

bool daemonize() {
    int fd;

    switch (fork()) {
    case -1:
        return false;
    case 0:
        break;
    default:
        exit(0);
    }

    if (setsid() == -1) {
        return false;
    }

    umask(0);

    if ((fd = open("/dev/null", O_RDWR)) == -1) {
        return false;
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
        return false;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        return false;
    }

    if (dup2(fd, STDERR_FILENO) == -1) {
        return false;
    }

    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            return false;
        }
    }

    return true;
}

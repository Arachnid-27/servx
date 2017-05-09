#include "inet.h"

#include <errno.h>
#include <unistd.h>

namespace servx {

IPAddress::IPAddress()
    : send_buf(-1), recv_buf(-1),
      backlog(5), fd(-1), reuseport(false) {
        memset(&addr_in, 0, sizeof(in_addr));
    }

IPAddress::~IPAddress() {
    if (fd != -1) {
        close_socket();
    }
}

bool IPAddress::set_addr(const std::string& s) {
    if (inet_pton(AF_INET, s.c_str(), &addr_in.sin_addr) == -1) {
        // err_log
        return false;
    }
    return true;
}

bool IPAddress::is_attr_equal(const std::shared_ptr<IPAddress>& other) {
    return send_buf == other->send_buf &&
           recv_buf == other->recv_buf &&
           backlog == other->backlog &&
           reuseport == other->reuseport;
}

int IPAddress::open_socket() {
    if (fd != -1) {
        return fd;
    }

    int on = 1;

    for (size_t i = 0; i < 5; ++i) {
        if (fd == -1) {
            fd = socket(AF_INET ,SOCK_STREAM | SOCK_NONBLOCK, 0);

            if (fd == -1) {
                return -1;
            }

            if (reuseport &&
                setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
                           &on, sizeof(int)) == -1) {
                // err_log
                break;
            }

            if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                           &on, sizeof(int)) == -1) {
                // err_log
                break;
            }
        }

        if (bind(fd, reinterpret_cast<sockaddr *>(&addr_in),
                 sizeof(sockaddr_in)) == -1) {
            if (errno == EADDRINUSE) {  // retry
                continue;
            }

            break;
        }

        if (listen(fd, backlog) == -1) {
            break;
        }

        if (recv_buf != -1 &&
            setsockopt(fd, SOL_SOCKET, SO_RCVBUF,
                       &recv_buf, sizeof(int)) == -1) {
            // err_log
        }

        if (send_buf != -1 &&
            setsockopt(fd, SOL_SOCKET, SO_SNDBUF,
                       &send_buf, sizeof(int)) == -1) {
            // err_log
        }

        return fd;
    }

    close_socket();
    return -1;
}

bool IPAddress::close_socket() {
    if (close(fd) == -1) {
        // err_log
        return false;
    }
    return true;
}

}

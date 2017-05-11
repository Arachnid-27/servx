#include "inet.h"

#include <errno.h>
#include <netdb.h>
#include <unistd.h>

namespace servx {

void IPSockAddr::set_addr(sockaddr *sa, uint8_t len) {
    memcpy(addr, sa, len);
    length = len;
}

uint16_t IPSockAddr::get_port() const {
    auto p = reinterpret_cast<const sockaddr*>(addr);

    if (p->sa_family == AF_INET6) {
        return reinterpret_cast<const sockaddr_in6*>(addr)->sin6_port;
    }

    return reinterpret_cast<const sockaddr_in*>(addr)->sin_port;
}

TcpSocket::TcpSocket()
    : send_buf(-1), recv_buf(-1),
      backlog(5), fd(-1), reuseport(false) {}

TcpSocket::~TcpSocket() {
    if (fd != -1) {
        close_socket();
    }
}

bool TcpSocket::init_addr(const std::string& s, const std::string& port) {
    addrinfo *res;
    addrinfo hint;

    memset(&hint, 0, sizeof(addrinfo));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_ADDRCONFIG | AI_NUMERICHOST | AI_NUMERICSERV;

    if (getaddrinfo(s.c_str(), port.c_str(), &hint, &res) == -1) {
        // err_log
        return false;
    }

    // just use the first one
    addr.set_addr(res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);

    return true;
}

bool TcpSocket::is_attr_equal(const std::shared_ptr<TcpSocket>& other) {
    return send_buf == other->send_buf &&
           recv_buf == other->recv_buf &&
           backlog == other->backlog &&
           reuseport == other->reuseport;
}

int TcpSocket::open_socket() {
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

        if (bind(fd, addr.get_sockaddr(), addr.get_length()) == -1) {
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

bool TcpSocket::close_socket() {
    if (close(fd) == -1) {
        // err_log
        return false;
    }
    return true;
}

}

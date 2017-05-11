#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <sys/socket.h>

#include "event.h"
#include "inet.h"

namespace servx {

class Event;

class Connection {
public:
    Connection();

    Connection(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection& operator=(Connection&&) = delete;

    ~Connection() = default;

    void open(int fd);

    void close();

    bool is_close() const { return socket_fd == -1; }

    void set_peer_sockaddr(sockaddr* sa, socklen_t len);

    int get_fd() const { return socket_fd; }

    Event* get_read_event() { return read_event; }

    Event* get_write_event() { return write_event; }

private:
    int socket_fd;
    IPSockAddr peer_addr;
    Event *read_event;
    Event *write_event;
};

inline void Connection::set_peer_sockaddr(sockaddr* sa, socklen_t len) {
    peer_addr.set_addr(sa, len);
}

}

#endif

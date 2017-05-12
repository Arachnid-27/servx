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

    bool open(int fd, bool lst = false);

    void close();

    bool is_close() const { return socket_fd == -1; }

    void set_peer_sockaddr(sockaddr* sa, socklen_t len);

    sockaddr* get_local_sockaddr() { return local_addr.get_sockaddr(); }

    int get_fd() const { return socket_fd; }

    Event* get_read_event() { return read_event; }

    Event* get_write_event() { return write_event; }

    uint64_t get_conn_id() const { return conn_id; }

    bool is_listen() const { return listen; }

private:
    uint64_t conn_id;
    int socket_fd;
    IPSockAddr peer_addr;
    IPSockAddr local_addr;
    Event *read_event;
    Event *write_event;
    bool listen;

    static uint64_t count;
};

inline void Connection::set_peer_sockaddr(sockaddr* sa, socklen_t len) {
    peer_addr.set_addr(sa, len);
}

}

#endif

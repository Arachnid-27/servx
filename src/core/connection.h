#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <sys/socket.h>

#include "buffer.h"
#include "inet.h"

namespace servx {

class Connection;

class Event {
public:
    Event(Connection* c, bool w);

    Event(const Event&) = delete;
    Event(Event&&) = delete;
    Event& operator=(const Event&) = delete;
    Event& operator=(Event&&) = delete;

    ~Event() = default;

    Connection* get_connection() const { return conn; }

    bool is_write_event() const { return write == 1; }

    bool is_active() const { return active == 1; }
    void set_active(bool a) { active = a; }

    time_t get_timer() const { return timer; }
    bool is_timer() const { return timer != 0; }
    void set_timer(time_t t) { timer = t; }

    bool is_ready() const { return ready; }
    void set_ready(bool r) { ready = r; }

    bool is_eof() const { return eof; }
    void set_eof(bool e) { eof = e; }

    bool is_error() const { return error; }
    void set_error(bool e) { error = e; }

    bool is_timeout() const { return timeout; }

    void handle() { handler(this); }
    void set_handler(const std::function<void(Event*)>& h) { handler = h; }

    void expire();

    void reset();

private:
    Connection *conn;
    uint32_t write:1;
    uint32_t active:1;
    uint32_t ready:1;
    uint32_t timeout:1;
    uint32_t eof:1;
    uint32_t error:1;
    time_t timer;
    std::function<void(Event*)> handler;
};

struct ConnectionContext {};

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

    Event* get_read_event() { return &read_event; }
    Event* get_write_event() { return &write_event; }

    uint64_t get_conn_id() const { return conn_id; }

    bool is_listen() const { return listen; }

    template <class T>
    void set_context(T* c) { ctx = std::unique_ptr<T>(c); }
    template <class T>
    T* get_context() const { return static_cast<T*>(ctx.get());}

    void init_recv_buf(int sz);

    Buffer* get_recv_buf() const { return recv_buf.get(); }
    Buffer* relase_recv_buf() { return recv_buf.release(); }

    bool is_timeout() const { return timeout; }
    void set_timeout(bool t) { timeout = t; }

private:
    uint64_t conn_id;
    int socket_fd;
    IPSockAddr peer_addr;
    IPSockAddr local_addr;
    Event read_event;
    Event write_event;
    std::unique_ptr<ConnectionContext> ctx;
    std::unique_ptr<Buffer> recv_buf;
    uint32_t listen:1;
    uint32_t timeout:1;

    static uint64_t count;
};

inline void Connection::set_peer_sockaddr(sockaddr* sa, socklen_t len) {
    peer_addr.set_addr(sa, len);
}

}

#endif

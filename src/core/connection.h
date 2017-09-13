#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <sys/socket.h>

#include <list>

#include "buffer.h"
#include "core.h"
#include "event.h"
#include "file.h"
#include "inet.h"
#include "io.h"

namespace servx {

struct ConnectionContext {
    virtual ~ConnectionContext() {}
};

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

    sockaddr* get_local_sockaddr() { return local_addr.get_sockaddr(); }

    int get_fd() const { return socket_fd; }

    Event* get_read_event() { return &read_event; }
    Event* get_write_event() { return &write_event; }

    uint64_t get_conn_id() const { return conn_id; }

    bool is_listen() const { return listen; }

    template <class T>
    T* get_context() const { return static_cast<T*>(ctx.get());}

    void set_context(ConnectionContext* p) {
        ctx = std::unique_ptr<ConnectionContext>(p);
    }

    Buffer* get_recv_buf() { return recv_buf.get(); }
    void init_recv_buf(int sz);

    bool is_timeout() const { return timeout; }
    void set_timeout(bool t) { timeout = t; }

    bool is_error() const { return error; }
    void set_error(bool e) { error = e; }

    int recv_data() {
        return recv_data(recv_buf.get(), recv_buf->get_remain());
    }

    int recv_data(Buffer* buf, uint32_t count);
    int send_data(Buffer* buf, uint32_t count);
    int send_file(File* file);

    template <typename Iter>
    Iter send_chain(Iter first, Iter last);

private:
    uint64_t conn_id;
    int socket_fd;
    IPSockAddr local_addr;
    Event read_event;
    Event write_event;
    std::unique_ptr<ConnectionContext> ctx;
    std::unique_ptr<Buffer> recv_buf;

    uint32_t listen:1;
    uint32_t timeout:1;
    uint32_t error:1;

    static uint64_t count;
};

template <typename Iter>
Iter Connection::send_chain(Iter first, Iter last) {
    struct iovec iovs[64];
    int total, cnt;
    Iter cur = first;

    while (cur != last) {
        total = cnt = 0;

        while (cur != last) {
            iovs[cnt].iov_base = static_cast<void*>((*cur)->get_pos());
            iovs[cnt].iov_len = (*cur)->get_size();
            total += (*cur)->get_size();
            ++cur;
            if (++cnt == 64) {
                break;
            }
        }

        int n = io_write_chain(socket_fd, iovs, cnt);

        if (n > 0) {
            if (n < total) {
                uint32_t num = n;
                while (first != cur) {
                    if (num < (*first)->get_size()) {
                        (*first)->move_pos(num);
                        break;
                    }
                    num -= (*first)->get_size();
                    ++first;
                }
            } else {
                std::advance(first, cnt);
                continue;
            }
        } else if (n == SERVX_ERROR) {
            error = 1;
        }

        write_event.set_ready(false);
        return first;
    }

    return last;
}

}

#endif

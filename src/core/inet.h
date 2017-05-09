#ifndef _INET_H_
#define _INET_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace servx {

class IPAddress {
public:
    IPAddress();

    IPAddress(const IPAddress&) = delete;

    IPAddress& operator=(const IPAddress&) = delete;

    ~IPAddress();

    bool set_addr(const std::string& s);

    int get_fd() const { return fd; }

    int get_port() { return addr_in.sin_port; }

    void set_port(int p) { addr_in.sin_port = htons(p); }

    void set_backlog(int s) { backlog = s; }

    bool is_reuseport() const { return reuseport; }

    void set_reuseport(bool r) { reuseport = r; }

    int get_send_buf() const { return send_buf; }

    void set_send_buf(int s) { send_buf = s; }

    int get_recv_buf() const { return recv_buf; }

    void set_recv_buf(int s) { recv_buf = s; }

    bool is_attr_equal(const std::shared_ptr<IPAddress>& other);

    bool is_addr_equal(const std::shared_ptr<IPAddress>& other);

    int open_socket();

private:
    bool close_socket();

private:
    sockaddr_in addr_in;
    int send_buf;
    int recv_buf;
    int backlog;
    int fd;
    bool reuseport;
};

inline bool IPAddress::is_addr_equal(
    const std::shared_ptr<IPAddress>& other) {
    return memcmp(&addr_in, &other->addr_in, sizeof(sockaddr_in)) == 0;
}

}

#endif

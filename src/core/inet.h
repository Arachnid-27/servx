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

class IPSockAddr {
public:
    IPSockAddr() = default;

    IPSockAddr(const IPSockAddr&) = delete;
    IPSockAddr(IPSockAddr&&) = delete;
    IPSockAddr& operator=(const IPSockAddr&) = delete;
    IPSockAddr& operator==(IPSockAddr&&) = delete;

    ~IPSockAddr() = default;

    void set_addr(sockaddr *sa, uint8_t len);

    sockaddr* get_sockaddr() { return reinterpret_cast<sockaddr*>(addr); }

    uint8_t get_length() const { return length; }

    uint16_t get_port() const;

    bool is_wildcard() const;

private:
    uint8_t length;
    char addr[sizeof(sockaddr_in6)];
};

uint16_t get_port_from_sockaddr(const sockaddr* addr);

class TcpSocket {
public:
    TcpSocket();

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket(TcpSocket&&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;
    TcpSocket& operator=(TcpSocket&&) = delete;

    ~TcpSocket();

    bool set_addr(const std::string& s);

    int get_fd() const { return fd; }

    uint16_t get_port() { return addr.get_port(); }

    void set_backlog(int s) { backlog = s; }

    void set_send_buf(int s) { send_buf = s; }

    void set_recv_buf(int s) { recv_buf = s; }

    bool is_deferred_accept() const { return deferred_accept; }
    void set_deferred_accept(bool d) { deferred_accept = d; }

    bool is_attr_equal(const std::shared_ptr<TcpSocket>& other);
    bool is_addr_equal(const std::shared_ptr<TcpSocket>& other);
    bool is_addr_equal(const sockaddr* other);

    bool is_wildcard() const { return addr.is_wildcard(); }

    bool init_addr(const std::string& s, const std::string& port);

    int open_socket();

private:
    bool close_socket();

private:
    IPSockAddr addr;
    int send_buf;
    int recv_buf;
    int backlog;
    int fd;
    bool deferred_accept;
};

inline bool TcpSocket::is_addr_equal(
    const std::shared_ptr<TcpSocket>& other) {
    return memcmp(addr.get_sockaddr(),
                  other->addr.get_sockaddr(), addr.get_length()) == 0;
}

inline bool TcpSocket::is_addr_equal(const sockaddr* other) {
    return memcmp(addr.get_sockaddr(), other, addr.get_length()) == 0;
}

}

#endif

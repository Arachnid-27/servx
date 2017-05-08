#ifndef _INET_H_
#define _INET_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <string>
#include <vector>

namespace servx {

class IPAddress {
public:
    IPAddress();

    bool set_addr(const std::string& s);

    int get_port() { return addr_in.sin_port; }

    void set_port(int p) { addr_in.sin_port = htons(p); }

    void set_backlog(int s) { backlog = s; }

    bool is_reuseport() const { return reuseport; }

    void set_reuseport(bool r) { reuseport = r; }

    int get_send_buf() const { return send_buf; }

    void set_send_buf(int s) { send_buf = s; }

    int get_recv_buf() const { return recv_buf; }

    void set_recv_buf(int s) { recv_buf = s; }

    bool is_attr_equal(IPAddress* other);

    bool is_addr_equal(IPAddress* other);

private:
    sockaddr_in addr_in;
    int send_buf;
    int recv_buf;
    int backlog;
    bool reuseport;
};

}

#endif

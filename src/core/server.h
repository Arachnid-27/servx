#ifndef _SERVER_H_
#define _SERVER_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstring>
#include <vector>
#include <string>

namespace servx {

class IPAddress {
public:
    IPAddress();

    bool set_addr(const std::string& s);

    void set_port(int p) { addr_in.sin_port = htons(p); }

    void set_backlog(int s) { backlog = s; }

    bool is_reuseport() const { return reuseport; }

    void set_reuseport(bool r) { reuseport = r; }

    int get_send_buf() const { return send_buf; }

    void set_send_buf(int s) { send_buf = s; }

    int get_recv_buf() const { return recv_buf; }

    void set_recv_buf(int s) { recv_buf = s; }

private:
    sockaddr_in addr_in;
    int send_buf;
    int recv_buf;
    int backlog;
    bool reuseport;
};

class Location {
public:
    Location(const char* s): uri(s) {}

    Location(std::string s): uri(s) {}

    Location(Location&&) = default;

private:
    std::string uri;
};

class Server {
public:
    void push_location(Location&& loc) { locations.push_back(std::move(loc)); }

    void new_address() { addresses.emplace_back(); }

    IPAddress& get_last_address() { return addresses.back(); }

private:
    std::vector<IPAddress> addresses;
    std::vector<Location> locations;
};

}

#endif

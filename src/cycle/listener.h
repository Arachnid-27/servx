#ifndef _LISTENER_H_
#define _LISTENER_H_

#include <memory>
#include <vector>
#include <unordered_map>

#include "connection.h"
#include "inet.h"
#include "server.h"

namespace servx {

class Listening {
public:
    Listening(const std::shared_ptr<TcpSocket>& s)
        : conn(nullptr), socket(s) {}

    Listening(const Listening&) = delete;
    Listening(Listening&&) = delete;
    Listening& operator=(const Listening&) = delete;
    Listening& operator=(Listening&&) = delete;

    ~Listening() = default;

    bool open_socket() { return socket->open_socket() != -1; }

    bool push_server(const std::shared_ptr<Server>& server, bool def);

    bool is_addr_equal(const std::shared_ptr<TcpSocket>& addr) const;

    bool is_attr_equal(const std::shared_ptr<TcpSocket>& addr) const;

    int get_fd() const { return socket->get_fd(); }

    void set_connection(Connection* c) { conn = c; }

    Connection* get_connection() { return conn; }

private:
    Connection *conn;
    std::shared_ptr<Server> default_server;
    std::vector<std::shared_ptr<Server>> servers;
    std::shared_ptr<TcpSocket> socket;
};

class Listener {
public:
    Listener(const Listener&) = delete;
    Listener(Listener&&) = delete;
    Listener& operator=(const Listener&) = delete;
    Listener& operator=(Listener&&) = delete;

    ~Listener() = default;

    bool push_address(const std::shared_ptr<TcpSocket>& addr,
                      const std::shared_ptr<Server>& server, bool def);

    bool init_listenings();

    bool init_reuseport_listenings();

    bool set_connection_to_listenings();

    bool enable_all();

    bool disable_all();

    static Listener* instance() { return listener; }

private:
    Listener() = default;

private:
    std::vector<std::shared_ptr<Listening>> listenings;
    std::vector<std::shared_ptr<Listening>> reuseport_listenings;
    std::unordered_map<int,
        std::vector<std::shared_ptr<Listening>>> addresses;

    static Listener *listener;
};

inline bool Listening::is_addr_equal(
    const std::shared_ptr<TcpSocket>& s) const {
    return socket->is_addr_equal(s);
}

inline bool Listening::is_attr_equal(
    const std::shared_ptr<TcpSocket>& s) const {
    return socket->is_attr_equal(s);
}

}

#endif

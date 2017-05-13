#ifndef _LISTENER_H_
#define _LISTENER_H_

#include <memory>
#include <vector>
#include <unordered_map>

#include "connection.h"
#include "inet.h"

namespace servx {

struct ListeningServers {};

class Listening {
public:
    Listening(const std::shared_ptr<TcpSocket>& s)
        : conn(nullptr), socket(s) {}

    Listening(const Listening&) = delete;
    Listening(Listening&&) = delete;
    Listening& operator=(const Listening&) = delete;
    Listening& operator=(Listening&&) = delete;

    ~Listening() = default;

    int open_socket() { return socket->open_socket(); }

    template <class T>
    void set_servers(T* p) { servers = std::move(std::unique_ptr<T>(p)); }

    ListeningServers* get_servers() { return servers.get(); }

    const std::shared_ptr<TcpSocket>& get_socket() const { return socket; }

    int get_fd() const { return socket->get_fd(); }

    void set_connection(Connection* c) { conn = c; }

    Connection* get_connection() { return conn; }

    void set_handler(const std::function<void(Connection*)> h) { handler = h; }

    void handle(Connection* c) { handler(c); }

private:
    // this is listen connection, not connect connection
    Connection *conn;
    std::function<void(Connection*)> handler;
    std::unique_ptr<ListeningServers> servers;
    std::shared_ptr<TcpSocket> socket;
};

class Listener {
public:
    Listener(const Listener&) = delete;
    Listener(Listener&&) = delete;
    Listener& operator=(const Listener&) = delete;
    Listener& operator=(Listener&&) = delete;

    ~Listener() = default;

    Listening* push_address(const std::shared_ptr<TcpSocket>& addr);

    bool init_listenings();

    bool open_listenings();

    bool enable_all();

    bool disable_all();

    Listening* find_listening(sockaddr* addr);

    static Listener* instance() { return listener; }

private:
    Listener() = default;

private:
    std::vector<Listening*> listenings;
    std::unordered_map<int,
        std::vector<std::unique_ptr<Listening>>> ports;

    static Listener *listener;
};

}

#endif

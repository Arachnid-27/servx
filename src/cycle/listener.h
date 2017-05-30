#ifndef _LISTENER_H_
#define _LISTENER_H_

#include <memory>
#include <vector>
#include <unordered_map>

#include "connection.h"
#include "inet.h"

namespace servx {

struct ListeningContext {
    virtual ~ListeningContext() {}
};

class Listening {
public:
    Listening(std::unique_ptr<TcpListenSocket>&& s)
        : conn(nullptr), socket(std::move(s)) {}

    Listening(const Listening&) = delete;
    Listening(Listening&&) = delete;
    Listening& operator=(const Listening&) = delete;
    Listening& operator=(Listening&&) = delete;

    ~Listening() = default;

    int listen() { return socket->listen(); }

    template <class T>
    T* get_context() { return static_cast<T*>(context.get()); }

    void set_context(ListeningContext* p) {
        context = std::unique_ptr<ListeningContext>(p);
    }

    TcpListenSocket* get_socket() const { return socket.get(); }

    int get_fd() const { return socket->get_fd(); }

    void set_connection(Connection* c) { conn = c; }

    Connection* get_connection() { return conn; }

    void set_handler(const std::function<void(Connection*)>& h) { handler = h; }

    void handle(Connection* c) { handler(c); }

private:
    Connection *conn;
    std::function<void(Connection*)> handler;
    std::unique_ptr<ListeningContext> context;
    std::unique_ptr<TcpListenSocket> socket;
};

class Listener {
public:
    Listener(const Listener&) = delete;
    Listener(Listener&&) = delete;
    Listener& operator=(const Listener&) = delete;
    Listener& operator=(Listener&&) = delete;

    ~Listener() = default;

    Listening* push_address(std::unique_ptr<TcpListenSocket>&& socket);

    bool init_listenings();
    bool open_listenings();

    bool enable_all();
    bool disable_all();

    Listening* find_listening(sockaddr* addr);

    static Listener* instance() { return listener; }

private:
    Listener() = default;

    std::vector<Listening*> listenings;
    std::unordered_map<int,
        std::vector<std::unique_ptr<Listening>>> ports;

    static Listener *listener;
};

}

#endif

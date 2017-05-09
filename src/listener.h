#ifndef _LISTENER_H_
#define _LISTENER_H_

#include <memory>
#include <vector>
#include <unordered_map>

#include "inet.h"
#include "server.h"

namespace servx {

class Listening {
public:
    Listening(const std::shared_ptr<IPAddress>& addr): address(addr) {}

    Listening(const Listening&) = delete;

    Listening& operator=(const Listening&) = delete;

    ~Listening() = default;

    bool open_socket() { return address->open_socket() != -1; }

    bool push_server(const std::shared_ptr<Server>& server, bool def);

    bool is_addr_equal(const std::shared_ptr<IPAddress>& addr) const;

    bool is_attr_equal(const std::shared_ptr<IPAddress>& addr) const;

    int get_fd() const { return address->get_fd(); }

private:
    std::shared_ptr<Server> default_server;
    std::vector<std::shared_ptr<Server>> servers;
    std::shared_ptr<IPAddress> address;
};

class Listener {
public:
    Listener(const Listener&) = delete;

    Listener& operator=(const Listener&) = delete;

    ~Listener() = delete;

    bool push_address(const std::shared_ptr<IPAddress>& addr,
                      const std::shared_ptr<Server>& server, bool def);

    bool init_listenings();

    bool init_reuseport_listenings();

    static Listener* instance() { return listener; }

private:
    Listener() = default;

    std::vector<std::shared_ptr<Listening>> listenings;
    std::vector<std::shared_ptr<Listening>> reuseport_listenings;
    std::unordered_map<int,
        std::vector<std::shared_ptr<Listening>>> addresses;

    static Listener *listener;
};

inline bool Listening::is_addr_equal(
    const std::shared_ptr<IPAddress>& addr) const {
    return address->is_addr_equal(addr);
}

inline bool Listening::is_attr_equal(
    const std::shared_ptr<IPAddress>& addr) const {
    return address->is_attr_equal(addr);
}

}

#endif

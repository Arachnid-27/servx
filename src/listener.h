#ifndef _LISTENER_H_
#define _LISTENER_H_

#include <vector>
#include <unordered_map>

#include "inet.h"
#include "listening.h"
#include "server.h"

namespace servx {

class HttpAddress {
public:
    HttpAddress(IPAddress* addr): address(addr) {}

    bool push_server(Server* server, bool def);

    IPAddress* get_address() const { return address; }

private:
    IPAddress *address;
    Server *default_server;
    std::vector<Server*> servers;
};

class Listener {
public:
    bool push_address(IPAddress* addr, Server* server, bool def);

    static Listener* instance() { return listener; }

private:
    std::vector<Listening> listenings;
    std::unordered_map<int, std::vector<HttpAddress>> addresses;

    static Listener *listener;
};

}

#endif

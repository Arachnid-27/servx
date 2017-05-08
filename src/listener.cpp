#include "listener.h"

namespace servx {

bool HttpAddress::push_server(Server* server, bool def) {
    if (def) {
        if (default_server != nullptr) {
            return false;
        }
        default_server = server;
    }
    servers.push_back(server);
    return true;
}

Listener* Listener::listener = new Listener;

bool Listener::push_address(IPAddress* addr, Server* server, bool def) {
    auto& vec = addresses[addr->get_port()];

    for (auto& s : vec) {
        if (addr->is_addr_equal(s.get_address())) {
            if (!addr->is_attr_equal(s.get_address())) {
                // ambiguous if addr:port is same but attr is different
                return false;
            }

            s.get_address()->set_reuseport(addr->is_reuseport());
            s.push_server(server, def);
            delete addr;    // just use the old IPAddress

            return true;
        }
    }

    HttpAddress ha = HttpAddress(addr);
    ha.push_server(server, def);
    vec.push_back(std::move(ha));

    return true;
}

}

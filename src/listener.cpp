#include "listener.h"

#include "connection_pool.h"
#include "event_module.h"

namespace servx {

bool Listening::push_server(const std::shared_ptr<Server>& server, bool def) {
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

bool Listener::push_address(const std::shared_ptr<TcpSocket>& addr,
                            const std::shared_ptr<Server>& server, bool def) {
    auto &vec = addresses[addr->get_port()];

    for (auto &lst : vec) {
        if (lst->is_addr_equal(addr)) {
            if (!lst->is_attr_equal(addr)) {
                // ambiguous if addr:port is same but attr is different
                return false;
            }

            lst->push_server(server, def);

            return true;
        }
    }

    auto lst = std::make_shared<Listening>(addr);
    lst->push_server(server, def);
    vec.push_back(lst);

    if (addr->is_reuseport()) {
        reuseport_listenings.emplace_back(lst);
    } else {
        listenings.emplace_back(lst);
    }

    return true;
}

bool Listener::init_listenings() {
    Connection *conn;

    for (auto &lst : listenings) {
        if (!lst->open_socket()) {
            return false;
        }

        conn = ConnectionPool::instance()
            ->get_connection(lst->get_fd(), true);

        if (conn == nullptr) {
            return false;
        }

        lst->set_connection(conn);
    }

    return true;
}

bool Listener::init_reuseport_listenings() {
    Connection *conn;

    for (auto &lst : reuseport_listenings) {
        if (!lst->open_socket()) {
            return false;
        }

        conn = ConnectionPool::instance()
            ->get_connection(lst->get_fd(), true);

        if (conn == nullptr) {
            return false;
        }

        lst->set_connection(conn);
    }

    return true;
}

bool Listener::enable_all() {
    Connection *conn;

    for (auto &lst : listenings) {
        conn = lst->get_connection();
        if (conn == nullptr || conn->get_read_event()->is_active()) {
            continue;
        }

        add_event(conn->get_read_event(), 0);
    }

    for (auto &lst : reuseport_listenings) {
        conn = lst->get_connection();
        if (conn == nullptr || conn->get_read_event()->is_active()) {
            continue;
        }

        add_event(conn->get_read_event(), 0);
    }

    return true;
}

bool Listener::disable_all() {
    Connection *conn;

    for (auto &lst : listenings) {
        conn = lst->get_connection();
        if (conn == nullptr || !conn->get_read_event()->is_active()) {
            continue;
        }

        del_event(conn->get_read_event(), 0);
    }

    for (auto &lst : reuseport_listenings) {
        conn = lst->get_connection();
        if (conn == nullptr || !conn->get_read_event()->is_active()) {
            continue;
        }

        del_event(conn->get_read_event(), 0);
    }

    return true;
}

}

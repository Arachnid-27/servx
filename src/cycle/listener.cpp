#include "listener.h"

#include <algorithm>

#include "connection_pool.h"
#include "event_module.h"
#include "logger.h"

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
bool Listener::push_address(const std::shared_ptr<TcpSocket>& socket,
                            const std::shared_ptr<Server>& server, bool def) {
    auto &vec = ports[socket->get_port()];

    for (auto &lst : vec) {
        if (socket->is_addr_equal(lst->get_socket())) {
            if (!socket->is_attr_equal(lst->get_socket())) {
                // ambiguous if addr:port is same but attr is different
                return false;
            }

            lst->push_server(server, def);

            return true;
        }
    }

    auto lst = std::make_shared<Listening>(socket);
    lst->push_server(server, def);
    vec.push_back(lst);
    listenings.push_back(lst);

    return true;
}

bool Listener::init_listenings() {
    // Todo delete same port and have wildcard address
    // we should make sure the wildcard address be bound in the end
    auto comp = [](const std::shared_ptr<Listening>& lhs,
                   const std::shared_ptr<Listening>& rhs)
        { return rhs->get_socket()->is_wildcard(); };

    for (auto &pr : ports) {
        std::sort(pr.second.begin(), pr.second.end(), comp);
        pr.second.shrink_to_fit();
    }

    std::sort(listenings.begin(), listenings.end(), comp);

    auto iter = listenings.cbegin();
    while (iter != listenings.cend()) {
        auto socket1 = (*iter)->get_socket();
        if (socket1->is_wildcard()) {
            break;
        }

        auto vec = ports[(*iter)->get_socket()->get_port()];
        auto socket2 = vec.back()->get_socket();
        if (socket2->is_wildcard() && socket1->is_attr_equal(socket2)) {
            iter = listenings.erase(iter);
        } else {
            ++iter;
        }
    }

    listenings.shrink_to_fit();

    return true;
}

bool Listener::open_listenings() {
    Connection *conn;

    for (auto &lst : listenings) {
        if (!lst->open_socket()) {
            Logger::instance()->error("open socket failed");
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

    return true;
}

std::shared_ptr<Listening> Listener::find_listening(sockaddr* addr) {
    auto iter = ports.find(get_port_from_sockaddr(addr));
    if (iter == ports.end()) {
        return nullptr;
    }

    for (auto &lst : iter->second) {
        if (lst->get_socket()->is_addr_equal(addr)) {
            return lst;
        }
    }

    return nullptr;
}

}

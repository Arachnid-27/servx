#include "listener.h"

#include <algorithm>

#include "accept.h"
#include "connection_pool.h"
#include "event_module.h"
#include "logger.h"

namespace servx {

Listener* Listener::listener = new Listener;

Listening* Listener::push_address(std::unique_ptr<TcpSocket>&& socket) {
    auto &vec = ports[socket->get_port()];

    for (auto &lst : vec) {
        if (socket->is_addr_equal(lst->get_socket())) {
            if (!socket->is_attr_equal(lst->get_socket())) {
                // ambiguous if addr:port is same but attr is different
                return nullptr;
            }

            return lst.get();
        }
    }

    vec.emplace_back(new Listening(std::move(socket)));
    listenings.push_back(vec.back().get());

    return listenings.back();
}

bool Listener::init_listenings() {
    // we must make sure the wildcard address be bound in the end
    // we don't care the order of other addresses

    auto comp1 = [](const std::unique_ptr<Listening>& lhs,
                    const std::unique_ptr<Listening>& rhs)
        { return rhs->get_socket()->is_wildcard(); };

    for (auto &pr : ports) {
        std::sort(pr.second.begin(), pr.second.end(), comp1);
        pr.second.shrink_to_fit();
    }

    auto comp2 = [](const Listening* lhs, const Listening* rhs)
        { return rhs->get_socket()->is_wildcard(); };

    std::sort(listenings.begin(), listenings.end(), comp2);

    auto iter = listenings.begin();
    while (iter != listenings.end()) {
        auto socket1 = (*iter)->get_socket();
        if (socket1->is_wildcard()) {
            break;
        }

        // delete the socket which have same attr and
        // port with the wildcard address
        // we only bind the wildcard address
        // we can find the real server by searching 'ports'
        auto &vec = ports[(*iter)->get_socket()->get_port()];
        auto socket2 = vec.back()->get_socket();
        if (socket2->is_wildcard() && socket1->is_attr_equal(socket2)) {
            iter = listenings.erase(iter);
        } else {
            ++iter;
        }
    }

    listenings.shrink_to_fit();
    Logger::instance()->debug("%d listenings in total", listenings.size());

    return true;
}

bool Listener::open_listenings() {
    Connection *conn;

    for (auto lst : listenings) {
        int fd = lst->open_socket();
        if (fd == -1) {
            Logger::instance()->error("open socket failed, errno %d", errno);
            return false;
        }

        conn = ConnectionPool::instance()
            ->get_connection(lst->get_fd(), true);
        if (conn == nullptr) {
            return false;
        }

        conn->get_read_event()->set_handler(
            [lst](Event* ev) { accept_event_handler(lst, ev); });

        if (!add_event(conn->get_read_event(), 0)) {
            Logger::instance()->error("can not add event");
            return false;
        }
        lst->set_connection(conn);
    }

    return true;
}

bool Listener::enable_all() {
    Connection *conn;

    for (auto lst : listenings) {
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

    for (auto lst : listenings) {
        conn = lst->get_connection();
        if (conn == nullptr || !conn->get_read_event()->is_active()) {
            continue;
        }

        del_event(conn->get_read_event(), 0);
    }

    return true;
}

Listening* Listener::find_listening(sockaddr* addr) {
    auto iter = ports.find(get_port_from_sockaddr(addr));
    if (iter == ports.end()) {
        Logger::instance()->alert("can not find port %d",
                                  get_port_from_sockaddr(addr));
        return nullptr;
    }

    for (auto &lst : iter->second) {
        if (lst->get_socket()->is_addr_equal(addr)) {
            return lst.get();
        }
    }

    // use the wildcard address
    if ((iter->second).back()->get_socket()->is_wildcard()) {
        return (iter->second).back().get();
    }

    return nullptr;
}

}

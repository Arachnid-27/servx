#include "http_listening.h"

#include "connection_pool.h"
#include "event_module.h"
#include "http_connection.h"
#include "timer.h"
#include "logger.h"

namespace servx {

bool HttpListening::push_server(Server* srv, bool def) {
    if (def) {
        if (default_server != nullptr) {
            Logger::instance()->error("default server exists!");
            return false;
        }
        default_server = srv;
    }
    servers.push_back(srv);

    return true;
}

Server* HttpListening::search_server(const std::string& name) {
    auto iter = std::find_if(servers.begin(), servers.end(),
        [&](const Server* srv) { return srv->contain_server_name(name); });

    if (iter != servers.end()) {
        return *iter;
    }
    return default_server;
}

void HttpListening::init_connection(Connection* conn) {
    if (!add_event(conn->get_read_event(), 0)) {
        Logger::instance()->error("add event failed");
        conn->close();
        return;
    }

    auto lst = Listener::instance()
        ->find_listening(conn->get_local_sockaddr());
    auto rev = conn->get_read_event();
    auto wev = conn->get_write_event();

    HttpListening *hl = lst->get_context<HttpListening>();
    HttpConnection *hc = new HttpConnection(hl);
    conn->set_context(hc);

    rev->set_handler([hc](Event* ev) { hc->wait_request(ev); });
    wev->set_handler(http_empty_write_handler);

    if (rev->is_ready()) { // deferred_accept
        rev->handle();
        return;
    }

    auto conf = hl->get_default_server()->get_core_conf();
    Timer::instance()->add_timer(conn->get_read_event(), conf->client_header_timeout);
    ConnectionPool::instance()->enable_reusable(conn);

    Logger::instance()->debug("init connection success!");
}

}

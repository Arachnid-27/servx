#include "http_connection.h"

#include "connection_pool.h"
#include "logger.h"
#include "timer.h"

namespace servx {

void HttpConnection::wait_request(Event* ev) {
    Connection *conn = ev->get_connection();
    if (ev->is_timeout()) {
        conn->close();
        return;
    }

    auto srv = listening->get_default_server();

    if (conn->get_recv_buf() == nullptr) {
        conn->init_recv_buf(srv->get_core_conf()->client_header_timeout);
    }

    int n = conn->recv_data();

    if (n == 0 || n == SERVX_ERROR) {
        if (n == 0) {
            Logger::instance()->info("client closed connection");
        }
        conn->close();
        return;
    }

    if (n == SERVX_AGAIN) {
        if (!ev->is_timer()) {
            Timer::instance()->add_timer(ev,
                srv->get_core_conf()->client_header_timeout);
        }
        ConnectionPool::instance()->enable_reusable(conn);
        return;
    }

    ConnectionPool::instance()->disable_reusable(conn);

    request = std::unique_ptr<HttpRequest>(new HttpRequest(conn));
    request->set_server(srv);

    Logger::instance()->debug("prepare to process request line");

    ev->set_handler([&](Event* ev) { request->process_line(ev); });
    request->process_line(ev);
}

}

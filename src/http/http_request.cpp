#include "http_request.h"

#include "event_module.h"
#include "timer.h"

namespace servx {

static void close_http_connection();

void http_wait_request_handler(Event* ev) {
    if (ev->is_timeout()) {
        close_http_connection();
        return;
    }

}

void http_empty_handler(Event* ev) {}

void http_init_connection(Connection* conn) {
    // we don't add write event until we send response
    if (!add_event(conn->get_read_event(), 0)) {
        close_http_connection();
        return;
    }

    auto lst = Listener::instance()
        ->find_listening(conn->get_local_sockaddr());
    auto rev = conn->get_read_event();

    rev->set_handler(http_wait_request_handler);
    conn->get_write_event()->set_handler(http_empty_handler);

    HttpConnection *hc = new HttpConnection;
    hc->set_server(static_cast<HttpServers*>(lst->get_servers())
        ->get_default_server());
    conn->set_context(hc);

    if (rev->is_ready()) { // deferred_accept
        rev->handle();
        return;
    }

    // Todo custom timeout
    Timer::instance()->add_timer(conn->get_read_event(), 60000);
    // Todo enable reuseable
}

static void close_http_connection() {
}

}

#include "http_request.h"

#include "event_module.h"
#include "io.h"
#include "logger.h"
#include "timer.h"

namespace servx {

HttpRequest::HttpRequest(Buffer* buf)
    : method(METHOD_UNKONWN), parse_state(0), recv_buf(buf) {
}

void close_http_connection(Connection* conn);

void http_wait_request_handler(Event* ev) {
    Connection *conn = ev->get_connection();
    if (ev->is_timeout()) {
        close_http_connection(conn);
        return;
    }

    if (conn->get_recv_buf() == nullptr) {
        // Todo custom the size of recv buffer
        conn->init_recv_buf(4096);
    }

    int n = recv(conn, 4096);

    if (n == -1) {
        if (ev->is_error()) {
            close_http_connection(conn);
            return;
        }

        // if event already exists in timer
        // it will delete the old one and insert
        Timer::instance()->add_timer(ev, 60000);

        // Todo enable reuse connection

        if (!ev->is_active()) {
            if (!add_event(ev, 0)) {
                close_http_connection(conn);
                return;
            }
        }

        return;
    }

    if (n == 0) {
        // first time recv 0 byte
        Logger::instance()->info("client closed connection");
        close_http_connection(conn);
        return;
    }

    // Todo disable reuse connection

    HttpRequest *req = new HttpRequest(conn->relase_recv_buf());
    // req->set_read_handler(http_block_reading);

    HttpConnection *hc = static_cast<HttpConnection*>(conn->get_context());
    hc->set_request(req);

    // prepare to process request line
    ev->set_handler(http_process_request_line);
    ev->handle();
}

static inline HttpConnection* get_http_connection(Connection* conn) {
    return static_cast<HttpConnection*>(conn->get_context());
}

void http_process_request_line(Event* ev) {
    Connection *conn = ev->get_connection();
    HttpRequest *req = get_http_connection(conn)->get_request();

    if (ev->is_timeout()) {
        Logger::instance()->info("%d client time out", HTTP_REQUEST_TIME_OUT);
        conn->set_timeout(true);
        req->close(HTTP_REQUEST_TIME_OUT);
        return;
    }
}

void http_empty_handler(Event* ev) {}

void http_init_connection(Connection* conn) {
    // we don't add write event until we send response
    if (!add_event(conn->get_read_event(), 0)) {
        close_http_connection(conn);
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
    // Todo enable reuse connection
}

void close_http_connection(Connection* conn) {
}

}

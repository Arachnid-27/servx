#include "http_request.h"

#include "event_module.h"
#include "http_parse.h"
#include "io.h"
#include "logger.h"
#include "timer.h"

namespace servx {

HttpRequest::HttpRequest(Buffer* buf)
    : http_method(METHOD_UNKONWN), parse_state(0),
      buf_offset(0), recv_buf(buf) {
}

std::string HttpRequest::get_headers_in(const char* s) const {
    auto iter = headers_in.find(std::string(s));
    if (iter == headers_in.end()) {
        return std::string("");
    }
    return iter->second;
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

    int rc = recv(conn, conn->get_recv_buf());

    if (rc == IO_ERROR) {
        close_http_connection(conn);
        return;
    }

    if (rc == IO_BLOCK) {
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

    if (rc == IO_FINISH) {
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

inline HttpConnection* get_http_connection(Connection* conn) {
    return static_cast<HttpConnection*>(conn->get_context());
}

int http_read_request_header(Event* ev, HttpRequest* req) {
    int rc = IO_BLOCK;

    if (ev->is_ready()) {
        while (true) {
            rc = recv(ev->get_connection(), req->get_recv_buf());

            if (rc == IO_BUF_TOO_SMALL) {
                if (req->get_recv_buf()->get_size() == 8192) {
                    Logger::instance()->info("request too large");
                    req->finalize(HTTP_BAD_REQUEST);
                } else {
                    req->get_recv_buf()->enlarge(8192);
                    continue;
                }
            }

            if (rc == IO_BLOCK) {
                break;
            }

            if (rc == IO_FINISH || rc == IO_ERROR) {
                if (rc == IO_FINISH) {
                    Logger::instance()->info("client prematurely closed connection");
                }
                req->finalize(HTTP_BAD_REQUEST);
            }

            return rc;
        }

    }

    Timer::instance()->add_timer(ev, 60000);

    if (!ev->is_active()) {
        if (!add_event(ev, 0)) {
            req->close(HTTP_INTERNAL_SERVER_ERROR);
            return IO_ERROR;
        }
    }

    return rc;
}

void http_process_request_headers(Event* ev) {
    Connection *conn = ev->get_connection();
    HttpRequest *req = get_http_connection(conn)->get_request();

    if (ev->is_timeout()) {
        Logger::instance()->info("%d client time out", HTTP_REQUEST_TIME_OUT);
        conn->set_timeout(true);
        req->close(HTTP_REQUEST_TIME_OUT);
        return;
    }

    int rc;

    while (true) {
        rc = http_read_request_header(ev, req);

        if (rc != IO_SUCCESS) {
            return;
        }

        rc = http_parse_request_headers(req);

        if (rc == PARSE_SUCCESS) {
            Logger::instance()->debug("parsing header success");

            auto host = req->get_headers_in("host");
            if (host.empty()) {
                req->finalize(HTTP_BAD_REQUEST);
                return;
            } else {
                // Todo find server
            }

            // Todo content-length
            // Todo chunk
            // Todo keep-alive
        }

        if (rc != PARSE_AGAIN) {
            req->finalize(HTTP_BAD_REQUEST);
            return;
        }
    }
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

    int rc;

    while (true) {
        rc = http_read_request_header(ev, req);

        if (rc != IO_SUCCESS) {
            return;
        }

        rc = http_parse_request_line(req);

        if (rc == PARSE_SUCCESS) {
            Logger::instance()->debug("parsing request success!\n"   \
                                      "method: %s\n"                 \
                                      "schema: %s\n"                 \
                                      "host: %s\n"                   \
                                      "uri: %s\n"                    \
                                      "args: %s\n"                   \
                                      "version: %s\n",
                                      req->get_method().c_str(),
                                      req->get_schema().c_str(),
                                      req->get_host().c_str(),
                                      req->get_uri().c_str(),
                                      req->get_args().c_str(),
                                      req->get_version().c_str());

            if (req->get_version() != "1.1") {
                req->finalize(HTTP_BAD_REQUEST);
                return;
            }

            // handle quoted
            if (req->is_quoted()) {
                if (http_parse_quoted(req) == PARSE_ERROR) {
                    req->finalize(HTTP_BAD_REQUEST);
                    return;
                }
            }

            // handle args
            if (!req->get_args().empty()) {
                if (http_parse_args(req) == PARSE_ERROR) {
                    req->finalize(HTTP_BAD_REQUEST);
                    return;
                }
            }

            if (!req->get_host().empty()) {
                // Todo check host format
            }

            ev->set_handler(http_process_request_headers);
            ev->handle();

            return;
        }

        if (rc != PARSE_AGAIN) {
            req->finalize(HTTP_BAD_REQUEST);
            return;
        }
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

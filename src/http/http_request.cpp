#include "http_request.h"

#include <cstdlib>

#include "connection_pool.h"
#include "event_module.h"
#include "http_parse.h"
#include "http_phase.h"
#include "io.h"
#include "logger.h"
#include "timer.h"

namespace servx {

HttpRequest::HttpRequest(Buffer* buf)
    : http_method(METHOD_UNKONWN), parse_state(0),
      buf_offset(0), content_length(-1),
      recv_buf(buf), server(nullptr),
      quoted(false), chunked(false), keep_alive(false) {
}

std::string HttpRequest::get_headers_in(const char* s) const {
    auto iter = headers_in.find(std::string(s));
    if (iter == headers_in.end()) {
        return std::string("");
    }
    return iter->second;
}

void http_request_handler(Event* ev) {
}

void http_wait_request_handler(Event* ev) {
    Connection *conn = ev->get_connection();
    if (ev->is_timeout()) {
        conn->close();
        return;
    }

    if (conn->get_recv_buf() == nullptr) {
        // Todo custom the size of recv buffer
        conn->init_recv_buf(4096);
    }

    Logger::instance()->debug("recv data...");

    int rc = recv(conn, conn->get_recv_buf());

    Logger::instance()->debug("recv return, get %d", rc);

    if (rc == IO_ERROR) {
        conn->close();
        return;
    }

    if (rc == IO_BLOCK) {
        // if event already exists in timer
        // it will delete the old one and insert
        Timer::instance()->add_timer(ev, 60000);
        ConnectionPool::instance()->enable_reusable(conn);

        if (!ev->is_active()) {
            if (!add_event(ev, 0)) {
                conn->close();
                return;
            }
        }

        return;
    }

    if (rc == IO_FINISH) {
        // first time recv 0 byte
        Logger::instance()->info("client closed connection");
        conn->close();
        return;
    }

    ConnectionPool::instance()->disable_reusable(conn);

    HttpRequest *req = new HttpRequest(conn->relase_recv_buf());
    // req->set_read_handler(http_block_reading);

    HttpConnection *hc = conn->get_context<HttpConnection>();
    hc->set_request(req);

    // prepare to process request line
    ev->set_handler(http_process_request_line);
    ev->handle();
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
                    Logger::instance()->info("client prematurely closed connetion");
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
    HttpRequest *req = conn->get_context<HttpConnection>()->get_request();

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
                auto ctx = conn->get_context<HttpConnection>();
                Server *srv = ctx->get_servers()->search_server(host);
                req->set_server(srv);
            }

            auto length = req->get_headers_in("content-length");
            if (!length.empty()) {
                int n = atoi(length.c_str());
                if (n == 0 && length != "0") {
                    req->finalize(HTTP_BAD_REQUEST);
                    return;
                }
                req->set_content_length(n);
            }

            auto encoding = req->get_headers_in("transfer-encoding");
            if (encoding == "chunked") {
                req->set_chunked(true);
            } else if (encoding != "identity") {
                Logger::instance()->error("unknown encoding %s",
                                          encoding.c_str());
                req->finalize(HTTP_NOT_IMPLEMENTED);
            }

            auto connection = req->get_headers_in("connection");
            if (connection == "keep-alive") {
                req->set_keep_alive(true);
            }

            if (conn->get_read_event()->is_timer()) {
                Timer::instance()->del_timer(conn->get_read_event());
            }

            conn->get_read_event()->set_handler(http_request_handler);
            conn->get_write_event()->set_handler(http_request_handler);
            req->set_read_handler(http_block_reading);
            req->set_write_handler(http_run_phases);

            return;
        }

        if (rc != PARSE_AGAIN) {
            req->finalize(HTTP_BAD_REQUEST);
            return;
        }
    }
}

void http_process_request_line(Event* ev) {
    Connection *conn = ev->get_connection();
    HttpRequest *req = conn->get_context<HttpConnection>()->get_request();

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

void http_init_connection(Connection* conn) {
    // we don't add write event until we send response
    if (!add_event(conn->get_read_event(), 0)) {
        Logger::instance()->error("can not add event");
        conn->close();
        return;
    }

    Logger::instance()->debug("find listening...");

    auto lst = Listener::instance()
        ->find_listening(conn->get_local_sockaddr());
    auto rev = conn->get_read_event();

    Logger::instance()->debug("find listening success, is_wildcard = %d",
                               lst->get_socket()->is_wildcard());

    rev->set_handler(http_wait_request_handler);

    HttpConnection *hc = new HttpConnection;
    hc->set_servers(lst->get_servers<HttpServers>());
    conn->set_context(hc);

    if (rev->is_ready()) { // deferred_accept
        rev->handle();
        return;
    }

    // Todo custom timeout
    Timer::instance()->add_timer(conn->get_read_event(), 60000);
    ConnectionPool::instance()->enable_reusable(conn);

    Logger::instance()->debug("init connection success");
}

void http_block_reading(HttpRequest* req) {}

}

#include "http_request.h"

#include <cstdlib>

#include "connection_pool.h"
#include "core.h"
#include "event_module.h"
#include "http_parse.h"
#include "io.h"
#include "logger.h"
#include "timer.h"

namespace servx {

HttpRequest::HttpRequest(Connection* c)
    : conn(c), http_method(HTTP_METHOD_UNKONWN), parse_state(0),
      buf_offset(0), content_length(-1), response(new HttpResponse(c)),
      phase_handler(0), content_handler(nullptr), server(nullptr),
      quoted(false), chunked(false), keep_alive(false) {
}

std::string HttpRequest::get_headers(const char* s) const {
    auto iter = headers.find(std::string(s));
    if (iter == headers.end()) {
        return std::string("");
    }
    return iter->second;
}

bool HttpRequest::discard_request_body() {
    if (discard_body) {
        return true;
    }

    if (!test_expect()) {
        return false;
    }

    // Todo recv data && set event handler

    discard_body = 1;

    return true;
}

bool HttpRequest::test_expect() {
    if (expect_tested) {
        return true;
    }

    auto expect = headers.find("expect");
    if (expect == headers.end()) {
        return true;
    }

    expect_tested = 1;

    if (expect->second != "100-continue") {
        return true;
    }

    static char response[] = "HTTP/1.1 100 Continue\r\n\r\n";
    if (conn->send_data(response, sizeof(response) - 1) == SERVX_OK) {
        return true;
    }

    // connection error

    return false;
}

int HttpRequest::read_request_header() {
    Event *ev = conn->get_read_event();
    int rc = SERVX_NOT_READY;

    if (ev->is_ready()) {
        while (true) {
            rc = conn->recv_data();

            if (rc == SERVX_DENY) {
                if (get_recv_buf()->get_size() == 8192) {
                    Logger::instance()->info("request too large");
                    finalize(HTTP_BAD_REQUEST);
                } else {
                    get_recv_buf()->enlarge(8192);
                    continue;
                }
            }

            if (rc == SERVX_NOT_READY) {
                break;
            }

            if (rc == SERVX_DONE) {
                Logger::instance()->info("client prematurely closed connection");
                finalize(HTTP_BAD_REQUEST);
            }

            if (rc == SERVX_ERROR) {
                finalize(HTTP_BAD_REQUEST);
            }

            return rc;
        }
    }

    if (!ev->is_timer()) {
        Timer::instance()->add_timer(ev, 60000);
    }

    if (!ev->is_active()) {
        if (!add_event(ev, 0)) {
            close(HTTP_INTERNAL_SERVER_ERROR);
            return SERVX_ERROR;
        }
    }

    return rc;
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

    int rc = conn->recv_data();

    if (rc == SERVX_ERROR) {
        conn->close();
        return;
    }

    if (rc == SERVX_NOT_READY) {
        if (!ev->is_timer()) {
            Timer::instance()->add_timer(ev, 60000);
        }
        ConnectionPool::instance()->enable_reusable(conn);

        if (!ev->is_active()) {
            if (!add_event(ev, 0)) {
                conn->close();
                return;
            }
        }

        return;
    }

    if (rc == SERVX_DONE) {
        // first time recv 0 byte
        Logger::instance()->info("client closed connection");
        conn->close();
        return;
    }

    ConnectionPool::instance()->disable_reusable(conn);

    HttpRequest *req = new HttpRequest(conn);
    // req->set_read_handler(http_block_reading);

    HttpConnection *hc = conn->get_context<HttpConnection>();
    hc->set_request(req);

    Logger::instance()->debug("prepare to process request line");

    // prepare to process request line
    ev->set_handler(http_process_request_line);
    ev->handle();
}

void http_process_request_headers(Event* ev) {
    Connection *conn = ev->get_connection();
    HttpRequest *req = conn->get_context<HttpConnection>()->get_request();
    Buffer *buf = req->get_recv_buf();

    if (ev->is_timeout()) {
        Logger::instance()->info("%d client time out", HTTP_REQUEST_TIME_OUT);
        conn->set_timeout(true);
        req->close(HTTP_REQUEST_TIME_OUT);
        return;
    }

    int rc;

    while (true) {
        if (buf->get_pos() + req->get_buf_offset() == buf->get_last()) {
            rc = req->read_request_header();
            if (rc != SERVX_OK) {
                return;
            }
        }

        rc = http_parse_request_headers(req);

        if (rc == SERVX_OK) {
            Logger::instance()->debug("parsing header success");

            auto host = req->get_headers("host");
            if (host.empty()) {
                Logger::instance()->error("can not find host");
                req->finalize(HTTP_BAD_REQUEST);
                return;
            } else {
                auto ctx = conn->get_context<HttpConnection>();
                Server *srv = ctx->get_servers()->search_server(host);
                req->set_server(srv);

                if (srv == ctx->get_servers()->get_default_server()) {
                    Logger::instance()->debug("use default server");
                }
            }

            auto length = req->get_headers("content-length");
            if (!length.empty()) {
                long n = atol(length.c_str());
                if (n == 0 && length != "0") {
                    req->finalize(HTTP_BAD_REQUEST);
                    return;
                }
                req->set_content_length(n);
            }

            auto encoding = req->get_headers("transfer-encoding");
            if (encoding == "chunked") {
                req->set_chunked(true);
            } else if (!encoding.empty() && encoding != "identity") {
                Logger::instance()->error("unknown encoding %s",
                                          encoding.c_str());
                req->finalize(HTTP_NOT_IMPLEMENTED);
                return;
            }

            auto connection = req->get_headers("connection");
            if (connection == "keep-alive") {
                Logger::instance()->debug("connection keep-alive");
                req->set_keep_alive(true);
                req->get_response()->set_keep_alive(true);
            }

            if (conn->get_read_event()->is_timer()) {
                Timer::instance()->del_timer(conn->get_read_event());
            }

            Logger::instance()->debug("prepare to run phases");

            conn->get_read_event()->set_handler(http_request_handler);
            conn->get_write_event()->set_handler(http_request_handler);
            req->set_read_handler(http_block_reading);
            req->set_write_handler([](HttpRequest* req)
                { HttpPhaseRunner::instance()->run(req); });

            return;
        }

        if (rc != SERVX_AGAIN) {
            req->finalize(HTTP_BAD_REQUEST);
            return;
        }
    }
}

void http_process_request_line(Event* ev) {
    Connection *conn = ev->get_connection();
    HttpRequest *req = conn->get_context<HttpConnection>()->get_request();
    Buffer *buf = req->get_recv_buf();

    if (ev->is_timeout()) {
        Logger::instance()->info("%d client time out", HTTP_REQUEST_TIME_OUT);
        conn->set_timeout(true);
        req->close(HTTP_REQUEST_TIME_OUT);
        return;
    }

    int rc;

    while (true) {
        if (buf->get_pos() + req->get_buf_offset() == buf->get_last()) {
            rc = req->read_request_header();
            if (rc != SERVX_OK) {
                return;
            }
        }

        Logger::instance()->debug("parsing request line...");

        rc = http_parse_request_line(req);

        if (rc == SERVX_OK) {
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
                if (http_parse_quoted(req) == SERVX_ERROR) {
                    req->finalize(HTTP_BAD_REQUEST);
                    return;
                }
            }

            // handle args
            if (!req->get_args().empty()) {
                if (http_parse_args(req) == SERVX_ERROR) {
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

        if (rc != SERVX_AGAIN) {
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

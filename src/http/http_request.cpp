#include "http_request.h"

#include <cstdlib>

#include "connection_pool.h"
#include "core.h"
#include "event_module.h"
#include "http_phase.h"
#include "http_parse.h"
#include "io.h"
#include "logger.h"
#include "timer.h"

namespace servx {

HttpRequest::HttpRequest(Connection* c)
    : conn(c), http_method(HTTP_METHOD_UNKONWN), parse_state(0),
      buf_offset(0), body(new HttpRequestBody(this)),
      response(new HttpResponse(c)),
      phase(HTTP_POST_READ_PHASE), phase_index(0),
      content_handler(nullptr), server(nullptr),
      quoted(false), keep_alive(false) {
}

std::string HttpRequest::get_headers(const char* s) const {
    auto iter = headers.find(std::string(s));
    if (iter == headers.end()) {
        return std::string("");
    }
    return iter->second;
}

int HttpRequest::read_request_header() {
    Event *ev = conn->get_read_event();

    if (ev->is_ready()) {
        if (get_recv_buf()->get_remain() == 0) {
            // TODO: enlarge double size
            Logger::instance()->error("request too large");
            finalize(HTTP_BAD_REQUEST);
            return SERVX_ERROR;
        }

        int n = conn->recv_data(); if (n == 0 || n == SERVX_ERROR) {
            if (n == 0) {
                Logger::instance()->
                    info("client permaturely closed connection");
            }
            finalize(HTTP_BAD_REQUEST);
            return SERVX_ERROR;
        }

        if (n == SERVX_AGAIN) {
            if (!ev->is_timer()) {
                Timer::instance()->add_timer(ev, 60000);
            }
            ConnectionPool::instance()->enable_reusable(conn);
            return SERVX_AGAIN;
        }

        return SERVX_OK;
    }

    return SERVX_AGAIN;
}

void HttpRequest::finalize(int rc) {
    Logger::instance()->debug("http finalize, rc = %d", rc);

    if (rc == SERVX_AGAIN) {
        return;
    }

    if (rc == SERVX_ERROR) {
        rc = HTTP_INTERNAL_SERVER_ERROR;
    }


    close(rc);
}

void HttpRequest::close(int status) {
    if (status != SERVX_OK) {
        response->set_content_length(0);
        response->set_status(status);
        response->send_header();
    }

    if (status == HTTP_REQUEST_TIME_OUT ||
        conn->get_read_event()->is_eof()) {
        return;
    }

    if (!keep_alive) {
        // TODO: linger close
        conn->close();
    } else {
        Timer::instance()->add_timer(conn->get_read_event(), 120000);
        conn->get_recv_buf()->shrink();
        conn->get_read_event()->set_ready(false);
        conn->get_read_event()->set_handler(http_wait_request_handler);
        conn->get_write_event()->set_ready(false);
        conn->get_write_event()->set_handler(http_empty_write_handler);
    }
}

void http_request_handler(Event* ev) {
    HttpRequest *req = ev->get_connection()->
        get_context<HttpConnection>()->get_request();

    // TODO: cancel dalay

    if (ev->is_write_event()) {
        req->handle_write();
    } else {
        req->handle_read();
    }

    // TODO: process subrequest
}

void http_wait_request_handler(Event* ev) {
    Connection *conn = ev->get_connection();
    if (ev->is_timeout()) {
        conn->close();
        return;
    }

    if (conn->get_recv_buf() == nullptr) {
        auto hc = conn->get_context<HttpConnection>();
        auto srv = hc->get_servers()->get_default_server();
        conn->init_recv_buf(srv->get_core_conf()->client_header_buffer_size);
    }

    Logger::instance()->debug("recv data...");

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
            Timer::instance()->add_timer(ev, 60000);
        }
        ConnectionPool::instance()->enable_reusable(conn);
        return;
    }

    ConnectionPool::instance()->disable_reusable(conn);

    HttpRequest *req = new HttpRequest(conn);

    HttpConnection *hc = conn->get_context<HttpConnection>();
    hc->set_request(req);

    Logger::instance()->debug("prepare to process request line");

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
                req->get_request_body()->set_content_length(n);
            }

            auto encoding = req->get_headers("transfer-encoding");
            if (!encoding.empty() && encoding != "identity") {
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

            req->handle_write();

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

            if (req->is_quoted()) {
                if (http_parse_quoted(req) == SERVX_ERROR) {
                    req->finalize(HTTP_BAD_REQUEST);
                    return;
                }
            }

            if (!req->get_args().empty()) {
                if (http_parse_args(req) == SERVX_ERROR) {
                    req->finalize(HTTP_BAD_REQUEST);
                    return;
                }
            }

            if (!req->get_host().empty()) {
                // TODO: check host format
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
    if (!add_event(conn->get_read_event(), 0)) {
        Logger::instance()->error("add event failed");
        conn->close();
        return;
    }

    auto lst = Listener::instance()
        ->find_listening(conn->get_local_sockaddr());
    auto rev = conn->get_read_event();
    auto wev = conn->get_write_event();

    Logger::instance()->debug("find listening success, is_wildcard = %d",
                               lst->get_socket()->is_wildcard());

    rev->set_handler(http_wait_request_handler);
    wev->set_handler(http_empty_write_handler);

    HttpServers *srvs = lst->get_servers<HttpServers>();
    conn->set_context(new HttpConnection(srvs));

    if (rev->is_ready()) { // deferred_accept
        rev->handle();
        return;
    }

    auto conf = srvs->get_default_server()->get_core_conf();
    Timer::instance()->add_timer(conn->get_read_event(), conf->client_header_timeout);
    ConnectionPool::instance()->enable_reusable(conn);

    Logger::instance()->debug("init connection success");
}

void http_block_reading(HttpRequest* req) {}

void http_block_writing(HttpRequest* req) {}

}

#ifndef _HTTP_UPSTREAM_REQUEST_H_
#define _HTTP_UPSTREAM_REQUEST_H_

#include <functional>
#include <list>
#include <unordered_map>

#include "buffer.h"
#include "connection.h"
#include "http_header.h"
#include "http_request.h"
#include "http_upstream_server.h"
#include "logger.h"

namespace servx {

class HttpUpstreamRequest {
public:
    HttpUpstreamRequest(HttpUpstreamServer* srv, HttpRequest* req,
        const std::function<int(HttpRequest*, Buffer*)>& rhh,
        const std::function<int(HttpRequest*, Buffer*)>& rbh,
        const std::function<void(HttpRequest*, int)>& fh)
        : server(srv), request(req),
          request_header_buf(nullptr),
          content_length(-1), recv(0),
          response_header_buf(nullptr),
          response_header_handler(rhh),
          response_body_handler(rbh),
          finalize_handler(fh) {}

    HttpUpstreamRequest(const HttpUpstreamRequest&) = delete;
    HttpUpstreamRequest(HttpUpstreamRequest&&) = delete;
    HttpUpstreamRequest& operator=(const HttpUpstreamRequest&) = delete;
    HttpUpstreamRequest& operator=(HttpUpstreamRequest&&) = delete;

    ~HttpUpstreamRequest();

    int connect();

    void set_extra_headers(const std::string& k, const std::string& v) {
        extra_headers[k] = v;
    }

    void wait_connect_handler(Event* ev);

    void send_request_handler(Event* ev);
    void recv_response_line_handler(Event* ev);
    void recv_response_headers_handler(Event* ev);
    void recv_response_body_handler(Event* ev);

    HttpResponseHeader* get_response_header() const { return header.get(); }

    void process_line();
    void process_headers();

    void close(int rc);

private:
    bool check_timeout(Event* ev, int rc) {
        if (ev->is_timeout()) {
            Logger::instance()->warn("upstream timeout");
            ev->get_connection()->set_timeout(true);
            close(rc);
            return true;
        }
        return false;
    }

    bool check_header_buf_full() {
        if (response_header_buf->get_remain() == 0) {
            Logger::instance()->error("response header too large");
            close(SERVX_ERROR);
            return true;
        }
        return false;
    }

    void response_header_done();
    void build_request();
    int handle_response_body();
    int read_response_header();
    int send_request();

    HttpUpstreamServer *server;
    HttpRequest *request;
    Connection* conn;

    std::unique_ptr<TcpConnectSocket> socket;

    std::unordered_map<std::string, std::string> extra_headers;

    std::unique_ptr<HttpResponseHeader> header;

    Buffer *request_header_buf;
    std::list<Buffer*> request_body_bufs;

    long content_length;
    long recv;

    Buffer *response_header_buf;
    std::list<Buffer*> response_body_bufs;

    std::function<int(HttpRequest*, Buffer*)> response_header_handler;
    std::function<int(HttpRequest*, Buffer*)> response_body_handler;
    std::function<void(HttpRequest*, int)> finalize_handler;
};

}

#endif

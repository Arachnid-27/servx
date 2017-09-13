#ifndef _HTTP_UPSTREAM_REQUEST_H_
#define _HTTP_UPSTREAM_REQUEST_H_

#include <functional>
#include <list>
#include <unordered_map>

#include "connection.h"
#include "http_header.h"
#include "http_upstream_server.h"
#include "http_upstream_response.h"
#include "inet.h"

namespace servx {

class HttpUpstreamRequest {
public:
    friend class HttpUpstreamResponse;

    HttpUpstreamRequest(HttpUpstreamServer* srv, HttpRequest* req,
        std::function<int(HttpRequest*, Buffer*)> rhh,
        std::function<int(HttpRequest*, Buffer*)> rbh,
        std::function<void(HttpRequest*, int)> fh)
        : server(srv), request(req), finished(false),
          response(this, rhh, rbh),
          request_header_buf(nullptr),
          finalize_handler(fh) {}

    HttpUpstreamRequest(const HttpUpstreamRequest&) = delete;
    HttpUpstreamRequest(HttpUpstreamRequest&&) = delete;
    HttpUpstreamRequest& operator=(const HttpUpstreamRequest&) = delete;
    HttpUpstreamRequest& operator=(HttpUpstreamRequest&&) = delete;

    ~HttpUpstreamRequest() = default;

    int connect();

    HttpUpstreamResponse& get_response() { return response; }

    void wait_connect_handler(Event* ev);
    void send_request_handler(Event* ev);

    void close(int rc);

    bool is_finished() const { return finished; }

private:
    bool check_timeout(Event* ev, int rc) {
        if (ev->is_timeout()) {
//            Logger::instance()->warn("upstream timeout");
            ev->get_connection()->set_timeout(true);
            close(rc);
            return true;
        }
        return false;
    }

    int build_request();
    int send_request();

    HttpUpstreamServer *server;
    HttpRequest *request;
    Connection* conn;

    bool finished;

    HttpUpstreamResponse response;

    Buffer *request_header_buf;
    std::list<Buffer*> request_body_bufs;

    std::function<void(HttpRequest*, int)> finalize_handler;
};

}

#endif

#ifndef _HTTP_UPSTREAM_REQUEST_H_
#define _HTTP_UPSTREAM_REQUEST_H_

#include <functional>
#include <list>
#include <unordered_map>

#include "buffer.h"
#include "connection.h"
#include "http_upstream_server.h"

namespace servx {

class HttpUpstreamRequest {
public:
    HttpUpstreamRequest(HttpUpstreamServer* srv,
        std::string&& m, std::string&& u, std::string&& a,
        const std::function<int(Buffer*)>& rh,
        const std::function<void(int)>& fh)
        : server(srv), method(m), uri(u), args(a),
          response_handler(rh), finalize_handler(fh) {}

    HttpUpstreamRequest(const HttpUpstreamRequest&) = delete;
    HttpUpstreamRequest(HttpUpstreamRequest&&) = delete;
    HttpUpstreamRequest& operator=(const HttpUpstreamRequest&) = delete;
    HttpUpstreamRequest& operator=(HttpUpstreamRequest&&) = delete;

    ~HttpUpstreamRequest() = default;

    int connect();

    void set_body(std::list<Buffer>&& b) { request_bufs = std::move(b); }
    void set_args(std::string&& a) { args = a; }
    void set_headers(std::string&& k, std::string&& v) { headers[k] = v; }

    void wait_connect_handler(Event* ev);

    void send_request_handler(Event* ev);
    void recv_response_handler(Event* ev);

    void close(int rc);

private:
    int send_request();

    HttpUpstreamServer* server;
    Connection* conn;

    std::string method;
    std::string uri;
    std::string args;

    std::unordered_map<std::string, std::string> headers;
    std::list<Buffer> request_bufs;
    std::list<Buffer*> response_bufs;

    std::function<int(Buffer*)> response_handler;
    std::function<void(int)> finalize_handler;
};

}

#endif

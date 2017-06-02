#ifndef _HTTP_UPSTREAM_REQUEST_H_
#define _HTTP_UPSTREAM_REQUEST_H_

#include <functional>
#include <list>
#include <unordered_map>

#include "buffer.h"
#include "connection.h"
#include "http_upstream_server.h"
#include "http_request.h"

namespace servx {

class HttpUpstreamRequest {
public:
    HttpUpstreamRequest(HttpUpstreamServer* srv, HttpRequest* req,
        const std::function<int(HttpRequest*, Buffer*)>& rh,
        const std::function<void(HttpRequest*, int)>& fh)
        : server(srv), request(req),
          response_handler(rh), finalize_handler(fh) {}

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
    void recv_response_handler(Event* ev);

    void close(int rc);

private:
    void build_request();
    int send_request();

    HttpUpstreamServer* server;
    HttpRequest *request;
    Connection* conn;

    std::unique_ptr<TcpConnectSocket> socket;

    std::unordered_map<std::string, std::string> extra_headers;
    std::list<Buffer*> request_header_bufs;
    std::list<Buffer*> request_body_bufs;
    std::list<Buffer*> response_bufs;

    std::function<int(HttpRequest*, Buffer*)> response_handler;
    std::function<void(HttpRequest*, int)> finalize_handler;
};

}

#endif

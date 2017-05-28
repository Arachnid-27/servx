#ifndef _HTTP_UPSTREAM_REQUEST_H_
#define _HTTP_UPSTREAM_REQUEST_H_

#include "http_upstream_server.h"

namespace servx {

class HttpUpstreamRequest {
public:
    HttpUpstreamRequest(HttpUpstreamServer* srv)
        : server(srv) {}

    HttpUpstreamRequest(const HttpUpstreamRequest&) = delete;
    HttpUpstreamRequest(HttpUpstreamRequest&&) = delete;
    HttpUpstreamRequest& operator=(const HttpUpstreamRequest&) = delete;
    HttpUpstreamRequest& operator=(HttpUpstreamRequest&&) = delete;

    ~HttpUpstreamRequest() = default;

    void connect();

private:
    HttpUpstreamServer* server;
};

}

#endif

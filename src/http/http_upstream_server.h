#ifndef _HTTP_UPSTREAM_SERVER_H_
#define _HTTP_UPSTREAM_SERVER_H_

#include "inet.h"

namespace servx {

class HttpUpstreamServer {
public:
    HttpUpstreamServer(std::unique_ptr<TcpSocket>&& s)
        : socket(std::move(s)) {}

    HttpUpstreamServer(const HttpUpstreamServer&) = delete;
    HttpUpstreamServer(HttpUpstreamServer&&) = delete;
    HttpUpstreamServer& operator=(const HttpUpstreamServer&) = delete;
    HttpUpstreamServer& operator=(HttpUpstreamServer&&) = delete;

    ~HttpUpstreamServer() = default;

    TcpSocket* get_socket() const { return socket.get(); }

private:
    std::unique_ptr<TcpSocket> socket;
};

}

#endif

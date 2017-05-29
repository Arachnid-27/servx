#ifndef _HTTP_UPSTREAM_SERVER_H_
#define _HTTP_UPSTREAM_SERVER_H_

#include "inet.h"

namespace servx {

class HttpUpstreamServer {
public:
    HttpUpstreamServer(std::unique_ptr<TcpConnectSocket>&& s)
        : socket(std::move(s)) {}

    HttpUpstreamServer(const HttpUpstreamServer&) = delete;
    HttpUpstreamServer(HttpUpstreamServer&&) = default;
    HttpUpstreamServer& operator=(const HttpUpstreamServer&) = delete;
    HttpUpstreamServer& operator=(HttpUpstreamServer&&) = default;

    ~HttpUpstreamServer() = default;

    TcpConnectSocket* get_socket() const { return socket.get(); }

private:
    std::unique_ptr<TcpConnectSocket> socket;
};

}

#endif

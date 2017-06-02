#ifndef _HTTP_UPSTREAM_SERVER_H_
#define _HTTP_UPSTREAM_SERVER_H_

#include "buffer.h"
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

    const TcpConnectSocket& get_socket() const { return *socket; }

    Buffer* get_body_buf();
    void ret_body_buf(Buffer* buf);

private:
    std::unique_ptr<TcpConnectSocket> socket;
    std::vector<Buffer> all_bufs;
    std::vector<Buffer*> free_body_bufs;
};

}

#endif

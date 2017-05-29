#include "http_upstream.h"

namespace servx {

bool HttpUpstream::push_server(
    const std::string& host, const std::string& port) {
    std::unique_ptr<TcpConnectSocket> socket(new TcpConnectSocket());
    if (!socket->init_addr(host, port, true)) {
        return false;
    }
    servers.emplace_back(std::move(socket));
    return true;
}

HttpUpstreamServer* HttpUpstream::get_server() {
    if (servers.size() != 1) {
        index = (index + 1) % servers.size();
    }
    return &servers[index];
}

}

#include "core.h"
#include "http_upstream.h"
#include "logger.h"

namespace servx {

bool HttpUpstream::push_server(
    const std::string& host, const std::string& port) {
    std::unique_ptr<TcpConnectSocket> socket(new TcpConnectSocket());
    socket->set_addr_str(host);
    socket->set_port_str(port);
    if (socket->init_addr(true) == SERVX_ERROR) {
        Logger::instance()->error("[upstream %s] push %s %s error",
            name.c_str(), host.c_str(), port.c_str());
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

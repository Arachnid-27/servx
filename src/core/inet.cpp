#include "inet.h"

#include <cstring>

namespace servx {

IPAddress::IPAddress()
    : send_buf(-1), recv_buf(-1),
      backlog(5), reuseport(false) {
        memset(&addr_in, 0, sizeof(in_addr));
    }

bool IPAddress::set_addr(const std::string& s) {
    if (inet_pton(AF_INET, s.c_str(), &addr_in.sin_addr) == -1) {
        // err_log
        return false;
    }
    return true;
}

bool IPAddress::is_attr_equal(IPAddress* other) {
    return send_buf == other->send_buf &&
           recv_buf == other->recv_buf &&
           backlog == other->backlog;
}

bool IPAddress::is_addr_equal(IPAddress* other) {
    return memcmp(&addr_in, &other->addr_in, sizeof(sockaddr_in)) == 0;
}

}

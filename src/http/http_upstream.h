#ifndef _HTTP_UPSTREAM_H_
#define _HTTP_UPSTREAM_H_

#include <string>
#include <vector>

#include "http_upstream_server.h"

namespace servx {

class HttpUpstream {
public:
    HttpUpstream(const std::string& s): index(0), name(s) {}

    HttpUpstream(const HttpUpstream&) = delete;
    HttpUpstream(HttpUpstream&&) = delete;
    HttpUpstream& operator=(const HttpUpstream&) = delete;
    HttpUpstream& operator=(HttpUpstream&&) = delete;

    ~HttpUpstream() = default;

    bool push_server(const std::string& host, const std::string& port);

    HttpUpstreamServer* get_server();

    const std::string& get_name() const { return name; }

    bool empty() const { return servers.empty(); }

private:
    uint32_t index;
    std::string name;
    std::vector<HttpUpstreamServer> servers;
};

}

#endif

#ifndef _HTTP_UPSTREAM_MODULE_H_
#define _HTTP_UPSTREAM_MODULE_H_

#include "http_module.h"
#include "http_upstream.h"

namespace servx {

struct HttpUpstreamConf {
    std::unordered_map<
        std::string, std::unique_ptr<HttpUpstream>> upstreams;
    std::unique_ptr<HttpUpstream> temp_upstream;
};

class HttpUpstreamModule: public HttpModule {
public:
    using srv_conf_t = void;
    using loc_conf_t = void;

    static HttpUpstreamConf conf;
    static HttpUpstreamModule instance;
    static std::vector<Command*> commands;
};

namespace command {

class Upstream: public Command {
public:
    Upstream(): Command("http", "upstream", 1) {}

    bool execute(const command_args_t& v) override;

    bool post_execute() override;
};

class UpServer: public Command {
public:
    UpServer(): Command("upstream", "server", 2) {}

    bool execute(const command_args_t& v) override;
};

}

}

#endif

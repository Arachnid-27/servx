#include "http_upstream_module.h"
#include "logger.h"

namespace servx {

HttpUpstreamConf HttpUpstreamModule::conf;
HttpUpstreamModule HttpUpstreamModule::instance;
std::vector<Command*> HttpUpstreamModule::commands = {
    new command::Upstream,
    new command::UpServer
};

namespace command {

bool Upstream::execute(const command_args_t& v) {
    if (v[0].empty()) {
        return false;
    }
    HttpUpstreamModule::conf.temp_upstream.reset(new HttpUpstream(v[0]));
    return true;
}

bool Upstream::post_execute() {
    auto &us = HttpUpstreamModule::conf.temp_upstream;
    if (us->empty()) {
        Logger::instance()->error("upstream %s is empty",
            HttpUpstreamModule::conf.temp_upstream->get_name().c_str());
        return false;
    }
    HttpUpstreamModule::conf.upstreams.emplace(us->get_name(), std::move(us));
    us = nullptr;
    return true;
}

bool UpServer::execute(const command_args_t& v) {
    return HttpUpstreamModule::conf.temp_upstream->push_server(v[0], v[1]);
}

}

}

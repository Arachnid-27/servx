#include "http_upstream_module.h"
#include "logger.h"

namespace servx {

std::unique_ptr<HttpUpstream> HttpUpstreamModule::upstream = nullptr;

int HttpUpstreamModule::upstream_handler(command_vals_t v) {
    if (v[0].empty()) {
        return SERVX_ERROR;
    }
    upstream = std::unique_ptr<HttpUpstream>(new HttpUpstream(v[0]));
    return UPSTREAM_BLOCK;
}

int HttpUpstreamModule::server_handler(command_vals_t v) {
    if (!upstream->push_server(v[0], v[1])) {
        return SERVX_ERROR;
    }
    return NULL_BLOCK;
}

bool HttpUpstreamModule::upstream_post_handler() {
    if (upstream->empty()) {
        Logger::instance()->error("upstream %s is empty",
            upstream->get_name().c_str());
        return false;
    }
    conf->upstreams.emplace(upstream->get_name(), std::move(upstream));
    upstream = nullptr;
    return true;
}

}

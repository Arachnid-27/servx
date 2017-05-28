#include "http_upstream_module.h"

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
    upstream->push_server(v[0], v[1]);
    return NULL_BLOCK;
}

bool HttpUpstreamModule::upstream_post_handler() {
    conf->upstreams.emplace(upstream->get_name(), std::move(upstream));
    upstream = nullptr;
    return true;
}

}

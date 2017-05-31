#include "http_proxy_module.h"

#include "http_core_module.h"
#include "http_upstream_module.h"
#include "http_upstream_request.h"
#include "logger.h"
#include "module_manager.h"

namespace servx {

int HttpProxyModule::proxy_pass_handler(command_vals_t v) {
    // TODO: not only upstream

    auto &upstreams = ModuleManager::instance()
        ->get_conf<HttpUpstreamModule>()->upstreams;
    auto iter = upstreams.find(v[0]);

    if (iter == upstreams.end()) {
        Logger::instance()->error(
            "can not find upstreams whose name is %s", v[0].c_str());
        return SERVX_ERROR;
    }

    auto conf = HttpCoreModule::get_cur_location()
        ->get_conf<HttpProxyModule>();
    conf->upstreams = iter->second.get();
    conf->url = v[0];

    return NULL_BLOCK;
}

int HttpProxyModule::proxy_pass_content_handler(HttpRequest* req) {
    auto conf = req->get_location()->get_conf<HttpProxyModule>();
    HttpUpstreamRequest *hur =
        new HttpUpstreamRequest(conf->upstreams->get_server(), req,
        proxy_pass_response_handler, proxy_pass_finalize_handler);

    auto ctx = req->get_context<HttpProxyModule>();
    ctx->hur = std::unique_ptr<HttpUpstreamRequest>(hur);

    return req->get_request_body()->read(proxy_pass_request_handler);
}

int HttpProxyModule::proxy_pass_request_handler(HttpRequest* req) {
    auto ctx = req->get_context<HttpProxyModule>();
    return ctx->hur->connect();
}

int HttpProxyModule::proxy_pass_response_handler(
    HttpRequest* req, Buffer* buf) {
    return SERVX_ERROR;
}

void HttpProxyModule::proxy_pass_finalize_handler(HttpRequest* req, int rc) {
}

}

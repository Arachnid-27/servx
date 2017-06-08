#include "http_proxy_module.h"

#include "event_module.h"
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

    auto loc = HttpCoreModule::get_cur_location();
    auto conf = loc->get_conf<HttpProxyModule>();

    loc->set_content_handler(proxy_pass_content_handler);
    conf->upstreams = iter->second.get();
    conf->url = v[0];

    return NULL_BLOCK;
}

int HttpProxyModule::proxy_pass_content_handler(HttpRequest* req) {
    Logger::instance()->debug("proxy pass content handler");

    auto conf = req->get_location()->get_conf<HttpProxyModule>();
    HttpUpstreamRequest *hur =
        new HttpUpstreamRequest(conf->upstreams->get_server(), req,
        proxy_pass_response_header_handler,
        proxy_pass_response_body_handler,
        proxy_pass_finalize_handler);

    auto ctx = new HttpProxyRequestContext;
    ctx->hur = std::unique_ptr<HttpUpstreamRequest>(hur);
    req->set_context<HttpProxyModule>(ctx);

    return req->get_request_body()->read(proxy_pass_request_handler);
}

int HttpProxyModule::proxy_pass_request_handler(HttpRequest* req) {
    Logger::instance()->debug("proxy pass request handler");

    auto ctx = req->get_context<HttpProxyModule>();
    return ctx->hur->connect();
}

int HttpProxyModule::proxy_pass_response_header_handler(
    HttpRequest* req, Buffer* buf) {
    // TODO: sometime we should buffer it
    int size = buf->get_size();
    int rc = req->get_connection()->send_data(buf, size);

    if (rc == SERVX_ERROR) {
        Logger::instance()->warn("proxy pass send header error");
        return SERVX_ERROR;
    }

    if (rc == SERVX_AGAIN || rc < size) {
        auto ctx = req->get_context<HttpProxyModule>();
        ctx->out.push_back(buf);
        req->set_write_handler(proxy_pass_write_handler);
        Event *ev = req->get_connection()->get_write_event();
        if (!ev->is_active() && !add_event(ev, 0)) {
            Logger::instance()->warn("add write event error");
            return SERVX_ERROR;
        }
        return SERVX_AGAIN;
    }

    return SERVX_OK;
}

int HttpProxyModule::proxy_pass_response_body_handler(
    HttpRequest* req, Buffer* buf) {
    auto ctx = req->get_context<HttpProxyModule>();
    if (!ctx->out.empty()) {
        ctx->out.push_back(buf);
        return SERVX_AGAIN;
    }

    int size = buf->get_size();
    int rc = req->get_connection()->send_data(buf, size);

    if (rc == SERVX_ERROR) {
        Logger::instance()->warn("proxy pass send header error");
        return SERVX_ERROR;
    }

    if (rc == SERVX_AGAIN || rc < size) {
        ctx->out.push_back(buf);
        req->set_write_handler(proxy_pass_write_handler);
        Event *ev = req->get_connection()->get_write_event();
        if (!ev->is_active() && !add_event(ev, 0)) {
            Logger::instance()->warn("add write event error");
            return SERVX_ERROR;
        }
        return SERVX_AGAIN;
    }

    return SERVX_OK;
}

void HttpProxyModule::proxy_pass_finalize_handler(HttpRequest* req, int rc) {
    auto ctx = req->get_context<HttpProxyModule>();
    if (rc == SERVX_OK && !ctx->out.empty()) {
        return;
    }
    req->close(rc);
}

void HttpProxyModule::proxy_pass_write_handler(HttpRequest* req) {
    auto ctx = req->get_context<HttpProxyModule>();
    auto &out = ctx->out;

    if (!out.empty()) {
        auto first = out.begin();
        auto last = out.end();
        auto iter = req->get_connection()->send_chain(first, last);
        if (req->get_connection()->is_error()) {
            Logger::instance()->info("connection error");
        }

        // FIXME return buffer

        out.erase(first, iter);

        if (iter == last) {
            req->close(SERVX_OK);
        }
    }
}

}

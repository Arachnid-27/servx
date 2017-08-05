#include "http_proxy_module.h"

#include "event_module.h"
#include "http_module.h"
#include "http_upstream_module.h"
#include "http_upstream_request.h"
#include "logger.h"
#include "module_manager.h"

namespace servx {

HttpProxyModule HttpProxyModule::instance;
std::vector<Command*> HttpProxyModule::commands = {
    new command::ProxyPass
};

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
    auto ctx = req->get_context<HttpProxyModule>();
    auto resp = ctx->hur->get_response_header();
    auto &location = resp->get_header("location");
    if (!location.empty()) {
        // 302
        auto &host = req->get_request_header()->get_header("host");
        auto first = location.begin() + 7;
        auto last = std::find(first, location.end(), '/');
        location.replace(first, last, host.c_str());
        // avoid refill buffer
        buf->reset();
        resp->fill_response_header(buf);
    }

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
        if (!ev->is_active() && !add_event(ev)) {
            Logger::instance()->warn("add write event error");
            return SERVX_ERROR;
        }
        return SERVX_AGAIN;
    }

    return SERVX_OK;
}

int HttpProxyModule::proxy_pass_response_body_handler(
    HttpRequest* req, Buffer* buf) {
    // TODO: sometime we should buffer it
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
        if (!ev->is_active() && !add_event(ev)) {
            Logger::instance()->warn("add write event error");
            return SERVX_ERROR;
        }
        return SERVX_AGAIN;
    }

    return SERVX_OK;
}

void HttpProxyModule::proxy_pass_finalize_handler(HttpRequest* req, int rc) {
    Logger::instance()->debug("proxy_pass finalize, rc = %d", rc);
    auto ctx = req->get_context<HttpProxyModule>();
    if (rc == SERVX_OK && !ctx->out.empty()) {
        return;
    }
    req->finalize(rc);
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

namespace command {

bool ProxyPass::execute(const command_args_t& v) {
    // TODO: not only upstream

    auto &upstreams = HttpUpstreamModule::conf.upstreams;
    auto iter = upstreams.find(v[0]);

    if (iter == upstreams.end()) {
        Logger::instance()->error(
            "can not find upstreams whose name is %s", v[0].c_str());
        return false;
    }

    auto &loc = HttpMainModule::conf.temp_location;
    loc->set_content_handler(HttpProxyModule::proxy_pass_content_handler);

    auto conf = loc->get_conf<HttpProxyModule>();
    conf->upstreams = iter->second.get();
    conf->url = v[0];

    return true;
}

}

}

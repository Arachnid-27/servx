#ifndef _HTTP_PROXY_MODULE_H_
#define _HTTP_PROXY_MODULE_H_

#include "http_module.h"
#include "http_upstream.h"
#include "http_upstream_request.h"

namespace servx {

struct HttpProxyLocConf: public HttpConf {
    std::string url;
    HttpUpstream* upstreams;
};

struct HttpProxyRequestContext: public HttpRequestContext {
    std::unique_ptr<HttpUpstreamRequest> hur;
    std::list<Buffer*> out;
};

class HttpProxyModule: public HttpModule {
public:
    using srv_conf_t = void;
    using loc_conf_t = HttpProxyLocConf;
    using request_context_t = HttpProxyRequestContext;

    virtual HttpConf* create_loc_conf() { return new HttpProxyLocConf(); }

    static HttpProxyModule instance;
    static std::vector<Command*> commands;

    static int proxy_pass_content_handler(HttpRequest* req);

    static int proxy_pass_request_handler(HttpRequest* req);

    static int proxy_pass_response_header_handler(
        HttpRequest* req, Buffer* buf);

    static int proxy_pass_response_body_handler(
        HttpRequest* req, Buffer* buf);

    static void proxy_pass_finalize_handler(HttpRequest* req, int rc);

    static void proxy_pass_write_handler(HttpRequest* req);

private:
    static int proxy_pass_write_out(Connection* conn, std::list<Buffer*>& out);
};

namespace command {

class ProxyPass: public Command {
public:
    ProxyPass(): Command("location", "proxy_pass", 1) {}

    bool execute(const command_args_t& v) override;
};

}

}

#endif

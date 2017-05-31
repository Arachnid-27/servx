#ifndef _HTTP_PROXY_MODULE_H_
#define _HTTP_PROXY_MODULE_H_

#include "http_module.h"
#include "http_upstream.h"
#include "http_upstream_request.h"

namespace servx {

struct HttpProxyLocConf: public ModuleConf {
    std::string url;
    HttpUpstream* upstreams;
};

struct HttpProxyRequestContext: public HttpRequestContext {
    std::unique_ptr<HttpUpstreamRequest> hur;
};

class HttpProxyModule
    : public HttpModuleWithConf<void, void,
                                HttpProxyLocConf,
                                HTTP_PROXY_MODULE,
                                HttpProxyRequestContext> {
public:
    HttpProxyModule(): HttpModuleWithConf(
        {
            new Command(LOCATION_BLOCK,
                        "proxy_pass",
                        lambda_handler(proxy_pass_handler), 1)
        }) {}

    int proxy_pass_handler(command_vals_t v);

    static int proxy_pass_content_handler(HttpRequest* req);

    static int proxy_pass_request_handler(HttpRequest* req);

    static int proxy_pass_response_handler(HttpRequest* req, Buffer* buf);

    static void proxy_pass_finalize_handler(HttpRequest* req, int rc);
};

}

#endif

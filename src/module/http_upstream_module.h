#ifndef _HTTP_UPSTREAM_MODULE_H_
#define _HTTP_UPSTREAM_MODULE_H_

#include "http_module.h"
#include "http_upstream.h"

namespace servx {

struct HttpUpstreamMainConf: public ModuleConf {
    std::unordered_map<
        std::string, std::unique_ptr<HttpUpstream>> upstreams;
};

class HttpUpstreamModule
    : public HttpModuleWithConf<HttpUpstreamMainConf,
                                void, void, HTTP_UPSTREAM_MODULE> {
public:
    HttpUpstreamModule(): HttpModuleWithConf(
        {
            new Command(HTTP_BLOCK,
                        "upstream",
                        lambda_handler(upstream_handler), 1,
                        lambda_post_handler(upstream_post_handler)),
            new Command(UPSTREAM_BLOCK,
                        "server",
                        lambda_handler(server_handler), 2)
        }) {}

    int upstream_handler(command_vals_t v);
    bool upstream_post_handler();

    int server_handler(command_vals_t v);

    static HttpUpstream* get_upstream() { return upstream.get(); }

private:
    static std::unique_ptr<HttpUpstream> upstream;
};

}

#endif

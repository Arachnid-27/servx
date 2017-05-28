#ifndef _HTTP_PROXY_MODULE_H_
#define _HTTP_PROXY_MODULE_H_

#include "http_module.h"

namespace servx {

struct HttpProxyLocConf: public ModuleConf {
    std::string url;
};

class HttpProxyModule
    : public HttpModuleWithConf<void, void, void, HTTP_PROXY_MODULE> {
public:
    HttpProxyModule(): HttpModuleWithConf(
        {
            new Command(LOCATION_BLOCK,
                        "proxy_pass",
                        lambda_handler(proxy_pass_handler), 1)
        }) {}

    int proxy_pass_handler(command_vals_t v);
};

}

#endif

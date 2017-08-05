#ifndef _HTTP_STATIC_MODULE_
#define _HTTP_STATIC_MODULE_

#include "http_module.h"
#include "http_request.h"

namespace servx {

class HttpStaticModule: public HttpModule {
public:
    using srv_conf_t = void;
    using loc_conf_t = void;

    bool post_configuration() override;

    static int http_static_handler(HttpRequest* req);

    static HttpStaticModule instance;
    static std::vector<Command*> commands;
};

}

#endif

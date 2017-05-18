#ifndef _HTTP_STATIC_MODULE_
#define _HTTP_STATIC_MODULE_

#include "http_module.h"
#include "http_request.h"

namespace servx {

class HttpStaticModule
    : public HttpModuleWithConf<void*, void, void, HTTP_STATIC_MODULE> {
public:
    HttpStaticModule(): HttpModuleWithConf({}) {}

    bool post_configuration() override;

    static int http_static_handler(HttpRequest* req);
};

}

#endif

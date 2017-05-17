#include "http_static_module.h"

#include "http_phase.h"
#include "logger.h"

namespace servx {

bool HttpStaticModule::post_configuration() {
    HttpPhaseRunner::instance()->register_handler(
        HTTP_CONTENT_PHASE, http_static_handler);
    return true;
}

int HttpStaticModule::http_static_handler(HttpRequest* req) {
    if (req->get_http_method() != HTTP_METHOD_GET &&
        req->get_http_method() != HTTP_METHOD_HEAD &&
        req->get_http_method() != HTTP_METHOD_POST) {
        return HTTP_NOT_ALLOWED;
    }

    // directory
    if (req->get_uri().back() == '/') {
        return HTTP_NEXT_HANDLER;
    }

    std::string path = req->get_location()->get_root() + req->get_uri();

    Logger::instance()->debug("http static file: %s", path.c_str());


    return HTTP_PHASE_SUCCESS;
}

}

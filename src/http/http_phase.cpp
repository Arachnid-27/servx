#include "http_phase.h"

#include "core.h"
#include "http_request.h"
#include "logger.h"

namespace servx {

HttpPhaseRunner* HttpPhaseRunner::runner = new HttpPhaseRunner;

void HttpPhaseRunner::init() {
    phase_handlers[HTTP_FIND_CONFIG_PHASE].push_back(find_config_handler);
}

void HttpPhaseRunner::run(HttpRequest* req) {
    int rc;

    while (true) {
        uint32_t phase = req->get_phase();
        auto &vec = phase_handlers[phase];

        if (vec.empty()) {
            req->next_phase();
            continue;
        }

        if (vec.size() <= req->get_phase_index()) {
            Logger::instance()->info("all %d phase handler deny!", phase);
            req->finalize(HTTP_FORBIDDEN);
            return;
        }

        switch (phase) {
        case HTTP_CONTENT_PHASE:
            rc = content_phase_checker(req);
            break;
        default:
            rc = generic_phase_checker(req);
            break;
        }

        Logger::instance()->debug("phase %d, get %d", phase, rc);

        if (rc == SERVX_OK) {
            return;
        }
    }
}

int HttpPhaseRunner::generic_phase_checker(HttpRequest* req) {
    auto &handler = phase_handlers[req->get_phase()][req->get_phase_index()];
    int rc = handler(req);

    Logger::instance()->debug("generic phase checker get %d", rc);

    if (rc == SERVX_DENY) {
        req->next_phase_index();
        return SERVX_AGAIN;
    }

    if (rc == SERVX_OK) {
        req->next_phase();
        return SERVX_AGAIN;
    }

    if (rc == SERVX_AGAIN) {
        return SERVX_OK;
    }

    req->finalize(rc);
    return SERVX_OK;
}

int HttpPhaseRunner::content_phase_checker(HttpRequest* req) {
    auto &vec = phase_handlers[req->get_phase()];
    auto handler = req->get_location()->get_content_handler();
    if (handler == nullptr) {
        handler = vec[req->get_phase_index()];
    }

    int rc =  handler(req);

    Logger::instance()->debug("content phase checker get %d", rc);

    if (rc == SERVX_AGAIN) {
        // set http_write
        return SERVX_OK;
    }

    if (rc != SERVX_DENY) {
        req->finalize(rc);
        return SERVX_OK;
    }

    if (req->get_phase_index() < vec.size() - 1) {
        req->next_phase_index();
        return SERVX_AGAIN;
    }

    if (req->get_request_header()->get_uri().back() == '/') {
        req->finalize(HTTP_FORBIDDEN);
        return SERVX_OK;
    }

    req->finalize(HTTP_NOT_FOUND);
    return SERVX_OK;
}

int HttpPhaseRunner::find_config_handler(HttpRequest* req) {
    Location *loc = req->get_server()->
        find_location(req->get_request_header()->get_uri());
    if (loc == nullptr) {
        Logger::instance()->info("can not find %s",
            req->get_request_header()->get_uri().c_str());
        return HTTP_NOT_FOUND;
    }

    long len = req->get_request_body()->get_content_length();
    if (len > 0 &&
        loc->get_core_conf()->client_max_body_size < len) {
        Logger::instance()->warn(
            "client body too large, %d bytes in tatol", len);
        return HTTP_REQUEST_ENTITY_TOO_LARGE;
    }

    req->set_location(loc);
    req->get_response()->set_location(loc);

    return SERVX_OK;
}

}

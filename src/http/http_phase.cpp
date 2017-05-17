#include "http_phase.h"

#include "logger.h"

namespace servx {

HttpPhaseRunner* HttpPhaseRunner::runner = new HttpPhaseRunner;

void HttpPhaseRunner::init() {
}

void HttpPhaseRunner::run(HttpRequest* req) {
    while (true) {
        size_t index = req->get_phase_handler();
        int rc = phase_handlers[index].check(req);

        if (rc == HTTP_CHECKER_OK) {
            return;
        }
    }
}

int HttpPhaseRunner::generic_phase_checker(
    HttpRequest* req, HttpPhaseHandler* ph) {
    int rc = ph->handle(req);

    if (rc == HTTP_NEXT_PHASE) {
        req->set_phase_handler(ph->get_next());
        return HTTP_CHECKER_AGAIN;
    }

    if (rc == HTTP_NEXT_HANDLER) {
        req->next_phase_handler();
        return HTTP_CHECKER_AGAIN;
    }

    if (rc == HTTP_PHASE_AGAIN) {
        return HTTP_CHECKER_OK;
    }

    req->finalize(rc);

    return HTTP_CHECKER_OK;
}

int HttpPhaseRunner::find_config_phase_checker(
    HttpRequest* req, HttpPhaseHandler* ph) {
    Location *loc = req->get_server()->find_location(req->get_uri());

    if (loc == nullptr) {
        req->finalize(HTTP_NOT_FOUND);
        return HTTP_CHECKER_OK;
    }

    if (req->get_content_length() > 0 &&
        loc->get_client_max_body_size() <
        static_cast<uint32_t>(req->get_content_length())) {
        Logger::instance()->warn("client body too large, %d bytes in tatol",
            req->get_content_length());
        req->finalize(HTTP_REQUEST_ENTITY_TOO_LARGE);
        return HTTP_CHECKER_OK;
    }

    req->next_phase_handler();
    return HTTP_CHECKER_AGAIN;
}

int HttpPhaseRunner::content_phase_checker(
    HttpRequest* req, HttpPhaseHandler* ph) {
    if (req->get_content_handler() != nullptr) {
        // Todo
        return HTTP_CHECKER_OK;
    }

    int rc = ph->handle(req);

    if (rc != HTTP_NEXT_HANDLER) {
        req->finalize(rc);
        return HTTP_CHECKER_OK;
    }

    if (req->get_phase_handler() < phase_handlers.size() - 1) {
        req->next_phase_handler();
        return HTTP_CHECKER_AGAIN;
    }

    if (req->get_uri().back() == '/') {
        req->finalize(HTTP_FORBIDDEN);
        return HTTP_CHECKER_OK;
    }

    req->finalize(HTTP_NOT_FOUND);
    return HTTP_CHECKER_OK;
}

}

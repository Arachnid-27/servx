#include "http_static_module.h"

#include "core.h"
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
        req->get_http_method() != HTTP_METHOD_HEAD) {
        return HTTP_NOT_ALLOWED;
    }

    // directory
    if (req->get_uri().back() == '/') {
        return SERVX_DENY;
    }

    std::string path = req->get_location()->get_root() + req->get_uri();

    Logger::instance()->debug("http static file: %s", path.c_str());

    std::unique_ptr<File> file =
        std::unique_ptr<File>(new File(std::move(path)));

    if (!file->open(O_RDONLY | O_NONBLOCK)) {
        return HTTP_NOT_FOUND;
    }

    if (!file->file_status()) {
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    HttpResponse *resp = req->get_response();

    if (file->is_dir()) {
        std::string location = file->get_pathname() + '/';
        resp->set_headers("Location", std::move(location));
        return HTTP_MOVED_PERMANENTLY;
    }

    if (!file->is_file()) {
        Logger::instance()->error("%s is not a regular file",
            file->get_pathname().c_str());
        return HTTP_NOT_FOUND;
    }

    if (!req->discard_request_body()) {
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    resp->set_status(HTTP_OK);
    resp->set_content_length(file->get_file_size());
    resp->set_last_modified_time(file->get_modify_time());
    resp->set_etag(true);

    // Todo set content_type

    int rc = resp->send_header();

    if (rc == SERVX_ERROR || resp->is_header_only()) {
        return rc;
    }

    return resp->send_body(std::move(file));
}

}

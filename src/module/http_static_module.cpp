#include "http_static_module.h"

#include "core.h"
#include "http_phase.h"
#include "logger.h"

namespace servx {

HttpStaticModule HttpStaticModule::instance;
std::vector<Command*> HttpStaticModule::commands;

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

    auto &uri = req->get_request_header()->get_uri();

    // directory
    if (uri.back() == '/') {
        return SERVX_DENY;
    }

    auto &root = req->get_location()->get_root();
    std::string path;

    if (root.empty()) {
        path = std::string(uri.begin() + 1, uri.end());
    } else {
        path = root + uri;
    }

    Logger::instance()->debug("http static file: %s", path.c_str());

    std::unique_ptr<File> file =
        std::unique_ptr<File>(new File(std::move(path)));

    if (!file->open(O_RDONLY | O_NONBLOCK)) {
        Logger::instance()->warn("open %s failed",
            file->get_pathname().c_str());
        return HTTP_NOT_FOUND;
    }

    if (!file->file_status()) {
        Logger::instance()->warn("can not get %s status",
            file->get_pathname().c_str());
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    HttpResponse *resp = req->get_response();

    if (file->is_dir()) {
        Logger::instance()->warn("%s is a directory",
            file->get_pathname().c_str());
        std::string location = path + '/';
        resp->set_headers("Location", std::move(location));
        return HTTP_MOVED_PERMANENTLY;
    }

    if (!file->is_file()) {
        Logger::instance()->error("%s is not a regular file",
            file->get_pathname().c_str());
        return HTTP_NOT_FOUND;
    }

    if (req->get_request_body()->discard() != SERVX_OK) {
        Logger::instance()->error("discard request body error");
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    resp->set_status(HTTP_OK);
    resp->set_content_length(file->get_file_size());
    resp->set_last_modified_time(file->get_modify_time());
    resp->set_etag(true);

    // TODO: set content_type

    Logger::instance()->debug("prepare to send header...");

    int rc = resp->send_header();

    if (rc == SERVX_ERROR || resp->is_header_only()) {
        return rc;
    }

    Logger::instance()->debug("prepare to send body...");

    return resp->send_body(std::move(file));
}

}

#include "http_response.h"

#include <cstdio>

#include "http_request.h"

namespace servx {

std::unordered_map<int, std::string> HttpResponse::status_lines = {
    { HTTP_CONTINUE, "100 Continue\r\n" },

    { HTTP_OK, "200 OK\r\n" },
    { HTTP_PARTIAL_CONTENT, "206 Partial Content\r\n" },

    { HTTP_MOVED_PERMANENTLY, "301 Moved Permanently\r\n" },
    { HTTP_MOVED_TEMPORARILY, "302 Moved Temporarily\r\n" },
    { HTTP_NOT_MODIFIED, "304 Not Modified\r\n" },

    { HTTP_BAD_REQUEST, "400 Bad Request\r\n" },
    { HTTP_UNAUTHORIZED, "401 Unauthorized\r\n" },
    { HTTP_FORBIDDEN, "403 Forbidden\r\n" },
    { HTTP_NOT_FOUND, "404 Not Found\r\n" },
    { HTTP_NOT_ALLOWED, "405 Not Allowed\r\n" },
    { HTTP_REQUEST_TIME_OUT, "408 Request Time-out\r\n" },
    { HTTP_CONFLICT, "409 Conflict\r\n" },
    { HTTP_LENGTH_REQUIRED, "411 Length Required\r\n" },
    { HTTP_PRECONDITION_FAILED, "412 Precondition Failed\r\n" },
    { HTTP_REQUEST_ENTITY_TOO_LARGE, "413 Request Entity Too Large\r\n" },
    { HTTP_REQUEST_URI_TOO_LARGE, "413 Request-URI Too Large\r\n" },

    { HTTP_INTERNAL_SERVER_ERROR, "500 Internal Server Error\r\n" },
    { HTTP_NOT_IMPLEMENTED, "501 Not Implemented\r\n" },
    { HTTP_BAD_GATEWAY, "502 Bad Gateway\r\n" },
    { HTTP_SERVICE_UNAVAILABLE, "503 Service Temporarily Unavailable\r\n" },
    { HTTP_GATEWAY_TIME_OUT, "504 Gateway Time-out\r\n" }
};

HttpResponse::HttpResponse()
    : send_buf(new Buffer(4096)), last_modified_time(-1),
      content_length(-1), status(-1) {
}

bool HttpResponse::set_etag() {
    if (last_modified_time == -1 || content_length == -1) {
        return false;
    }

    char buf[128];
    sprintf(buf, "\"%lx-%lx\"", last_modified_time, content_length);
    headers.emplace("ETag", buf);

    return true;
}

bool HttpResponse::send_header() {
    if (status == -1) {
        return false;
    }

    if (status != HTTP_OK &&
        status != HTTP_PARTIAL_CONTENT &&
        status != HTTP_NOT_MODIFIED) {
        last_modified_time = -1;
    }

    // ...

    return true;
}

}

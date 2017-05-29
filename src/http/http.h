#ifndef _HTTP_H_
#define _HTTP_H_

#define CR      '\r'
#define LF      '\n'
#define CRLF    "\r\n"

#include "connection.h"

namespace servx {

enum HttpPhase {
    HTTP_POST_READ_PHASE,
//  HTTP_REWRITE_PHASE
    HTTP_FIND_CONFIG_PHASE,
//  HTTP_ACCESS_PHASE,
    HTTP_CONTENT_PHASE,
    HTTP_LOG_PHASE
};

enum HttpMethod {
    HTTP_METHOD_UNKONWN,
    HTTP_METHOD_GET,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_OPTIONS
};

enum HttpStateCode {
    HTTP_CONTINUE = 10,             // 100

    HTTP_OK,                        // 200
    HTTP_PARTIAL_CONTENT,           // 206

    HTTP_MOVED_PERMANENTLY,         // 301
    HTTP_MOVED_TEMPORARILY,         // 302
    HTTP_NOT_MODIFIED,              // 304

    HTTP_BAD_REQUEST,               // 400
    HTTP_UNAUTHORIZED,              // 401
    HTTP_FORBIDDEN,                 // 403
    HTTP_NOT_FOUND,                 // 404
    HTTP_NOT_ALLOWED,               // 405
    HTTP_REQUEST_TIME_OUT,          // 408
    HTTP_CONFLICT,                  // 409
    HTTP_LENGTH_REQUIRED,           // 411
    HTTP_PRECONDITION_FAILED,       // 412
    HTTP_REQUEST_ENTITY_TOO_LARGE,  // 413
    HTTP_REQUEST_URI_TOO_LARGE,     // 414

    HTTP_INTERNAL_SERVER_ERROR,     // 500
    HTTP_NOT_IMPLEMENTED,           // 501
    HTTP_BAD_GATEWAY,               // 502
    HTTP_SERVICE_UNAVAILABLE,       // 503
    HTTP_GATEWAY_TIME_OUT           // 504
};

}

#endif

#ifndef _HTTP_PARSE_H_
#define _HTTP_PARSE_H_

#include "http_request.h"

namespace servx {

enum HttpParseResult {
    PARSE_OK,
    PARSE_AGAIN,
    PARSE_INVALID_METHOD,
    PARSE_INVALID_REQUEST,
    PARSE_ERROR
};

int http_parse_request_line(HttpRequest* req);

}

#endif

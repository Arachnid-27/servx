#ifndef _HTTP_PARSE_H_
#define _HTTP_PARSE_H_

#include "http_request.h"

namespace servx {

enum HttpParseResult {
    PARSE_OK,
    PARSE_AGAIN,
    PARSE_ERROR
};

enum HttpParseState {
    PARSE_START,
    PARSE_METHOD,
    PARSE_SPACE,
    PARSE_SCHEMA
};

int http_parse_request_line(HttpRequest* req);

}

#endif

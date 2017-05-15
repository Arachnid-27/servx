#ifndef _HTTP_PARSE_H_
#define _HTTP_PARSE_H_

#include "http_request.h"

namespace servx {

enum HttpParseResult {
    PARSE_SUCCESS,
    PARSE_AGAIN,
    PARSE_ERROR
};

int http_parse_request_line(HttpRequest* req);

int http_parse_request_headers(HttpRequest* req);

int http_parse_quoted(HttpRequest* req);

int http_parse_args(HttpRequest* req);

}

#endif

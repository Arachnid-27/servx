#include "http_parse.h"

#define LF      '\n'
#define CR      '\r'
#define CRLF    "\r\n"

#define LOWER_CASE(ch) (ch | 0x20)

namespace servx {

inline int compare_method_4(const char* p1, const char* p2) {
    return *reinterpret_cast<const uint32_t*>(p1) ==
           *reinterpret_cast<const uint32_t*>(p2);
}

inline int compare_method_6(const char* p1, const char* p2) {
    return *reinterpret_cast<const uint32_t*>(p1) ==
           *reinterpret_cast<const uint32_t*>(p2) &&
           *reinterpret_cast<const uint16_t*>(p1 + 4) ==
           *reinterpret_cast<const uint16_t*>(p2 + 4);
}

inline int compare_method_8(const char* p1, const char* p2) {
    return *reinterpret_cast<const uint32_t*>(p1) ==
           *reinterpret_cast<const uint32_t*>(p2) &&
           *reinterpret_cast<const uint32_t*>(p1 + 4) ==
           *reinterpret_cast<const uint32_t*>(p2 + 4);
}

enum HttpParseState {
    PARSE_START,

    PARSE_METHOD,
    PARSE_BEFORE_URI,
    PARSE_ABSOLUTE_URI_SCHEMA,
    PARSE_ABSOLUTE_URI_SCHEMA_SLASH,
    PARSE_ABSOLUTE_URI_SCHEMA_SLASH_2,
    PARSE_ABSOLUTE_URI_HOST,
    PARSE_ABSOLUTE_URI_HOST_IPV6,
    PARSE_ABSOLUTE_URI_HOST_IPV4_OR_REG_NAME,
    PARSE_ABSOLUTE_URI_HOST_END,
    PARSE_ABSOLUTE_URI_HOST_COLON,
    PARSE_ABSOLUTE_URI_PORT,
    PARSE_ABS_PATH,
    PARSE_ABS_PATH_QUESTION,
    PARSE_ABS_PATH_ARGS,
    PARSE_ABS_PATH_FRAGMENT,
    PARSE_VERSION_H,
    PARSE_VERSION_HT,
    PARSE_VERSION_HTT,
    PARSE_VERSION_HTTP,
    PARSE_VERSION_HTTP_SLASH,
    PARSE_VERSION_HTTP_SLASH_NUMBER,
    PARSE_VERSION_HTTP_SLASH_VERSION,

    PARSE_HEADERS_NAME,
    PARSE_HEADERS_COLON,
    PARSE_HEADERS_VALUE,

    PARSE_LAST_CR,
    PARSE_LAST_CR_LF,
    PARSE_LAST_CR_LF_CR,
    PARSE_LAST_CR_LF_CR_LF,

    PARSE_DONE
};

void http_parse_method(HttpRequest* req, const char* p1, const char *p2) {
    switch (p2 - p1) {
    case 3:
    case 4:
        if (compare_method_4(p1, "GET ")) {
            req->set_method(METHOD_GET);
            break;
        }

        if (compare_method_4(p1, "HEAD")) {
            req->set_method(METHOD_HEAD);
            break;
        }

        if (compare_method_4(p1, "POST")) {
            req->set_method(METHOD_POST);
            break;
        }

        if (compare_method_4(p1, "PUT ")) {
            req->set_method(METHOD_PUT);
            break;
        }

        break;
    case 6:
        if (compare_method_6(p1, "DELETE")) {
            req->set_method(METHOD_DELETE);
        }

        break;
    case 7:
        if (compare_method_8(p1, "OPTIONS ")) {
            req->set_method(METHOD_OPTIONS);
        }

        break;
    }
}

int http_parse_request_line(HttpRequest* req) {
    static const char* slash = "/";

    int state = req->get_parse_state();
    Buffer *buf = req->get_recv_buf();
    char *last = buf->get_last();
    char c, ch;

    for (char *p = buf->get_pos() + req->get_buf_offset(); p < last; ++p) {
        ch = *p;

        switch (state) {
        case PARSE_START:
            if (ch == CR || ch == LF) {
                break;
            }

            if (ch >= 'A' && ch <= 'Z') {
                state = PARSE_METHOD;
                break;
            }

            return PARSE_ERROR;

        case PARSE_METHOD:
            if (ch >= 'A' && ch <= 'Z') {
                break;
            }

            if (ch == ' ') {
                http_parse_method(req, buf->get_pos(), p);
                req->set_method(buf->get_pos(), p);
                state = PARSE_BEFORE_URI;
                break;
            }

            return PARSE_ERROR;

        case PARSE_BEFORE_URI:
            if (ch == '/') {
                buf->set_pos(p);
                state = PARSE_ABS_PATH;
                break;
            }

            c = LOWER_CASE(ch);
            if (c >= 'a' && c <= 'z') {
                buf->set_pos(p);
                state = PARSE_ABSOLUTE_URI_SCHEMA;
                break;
            }

            if (ch == ' ') {
                break;
            }

            return PARSE_ERROR;

        case PARSE_ABSOLUTE_URI_SCHEMA:
            c = LOWER_CASE(ch);
            if (c >= 'a' && c <= 'z') {
                break;
            }

            if (ch == ':') {
                req->set_schema(buf->get_pos(), p);
                state = PARSE_ABSOLUTE_URI_SCHEMA_SLASH;
                break;
            }

            return PARSE_ERROR;

        case PARSE_ABSOLUTE_URI_SCHEMA_SLASH:
            if (ch == '/') {
                state = PARSE_ABSOLUTE_URI_SCHEMA_SLASH_2;
                break;
            }

            return PARSE_ERROR;

        case PARSE_ABSOLUTE_URI_SCHEMA_SLASH_2:
            if (ch == '/') {
                state = PARSE_ABSOLUTE_URI_HOST;
                break;
            }

            return PARSE_ERROR;

        case PARSE_ABSOLUTE_URI_HOST:   // rfc 3986 3.2.2
            buf->set_pos(p);

            if (ch == '[') {
                state = PARSE_ABSOLUTE_URI_HOST_IPV6;
                break;
            }

            state = PARSE_ABSOLUTE_URI_HOST_IPV4_OR_REG_NAME;

            // fall through

        case PARSE_ABSOLUTE_URI_HOST_IPV4_OR_REG_NAME:
            if ((ch >= '0' && ch <= '9') || ch == '.' || ch == '-') {
                break;
            }

            c = LOWER_CASE(ch);
            if (c >= 'a' && c <= 'z') {
                break;
            }

            // fall through

        case PARSE_ABSOLUTE_URI_HOST_END:
            switch (ch) {
            case ':':
                req->set_host(buf->get_pos(), p);
                state = PARSE_ABSOLUTE_URI_HOST_COLON;
                break;
            case '/':
                req->set_host(buf->get_pos(), p);
                buf->set_pos(p);
                state = PARSE_ABS_PATH;
                break;
            case ' ':
                req->set_host(buf->get_pos(), p);
                req->set_uri(slash, slash + 1);
                state = PARSE_VERSION_H;
                break;
            default:
                return PARSE_ERROR;
            }

            break;

        case PARSE_ABSOLUTE_URI_HOST_IPV6:
            if ((ch >= '0' && ch <= '9') || ch == ':' || ch == '.') {
                break;
            }

            c = LOWER_CASE(ch);
            if (c >= 'a' && c <= 'f') {
                break;
            }

            if (ch == ']') {
                state = PARSE_ABSOLUTE_URI_HOST_END;
                break;
            }

            return PARSE_ERROR;

        case PARSE_ABSOLUTE_URI_HOST_COLON:
            if (ch >= '0' && ch <= '9') {
                buf->set_pos(p);
                state = PARSE_ABSOLUTE_URI_PORT;
                break;
            }

            switch (ch) {
            case '/':
                buf->set_pos(p);
                state = PARSE_ABS_PATH;
                break;
            case ' ':
                req->set_uri(slash, slash + 1);
                state = PARSE_VERSION_H;
                break;
            default:
                return PARSE_ERROR;
            }

            break;

        case PARSE_ABSOLUTE_URI_PORT:
            if (ch >= '0' && ch <= '9') {
                break;
            }

            switch (ch) {
            case '/':
                req->set_port(buf->get_pos(), p);
                buf->set_pos(p);
                state = PARSE_ABS_PATH;
                break;
            case ' ':
                req->set_port(buf->get_pos(), p);
                req->set_uri(slash, slash + 1);
                state = PARSE_VERSION_H;
                break;
            default:
                return PARSE_ERROR;
            }

            break;

        case PARSE_ABS_PATH:
            switch (ch) {
            case ' ':
                req->set_uri(buf->get_pos(), p);
                state = PARSE_VERSION_H;
                break;
            case '#':
                req->set_uri(buf->get_pos(), p);
                state = PARSE_ABS_PATH_FRAGMENT;
                break;
            case '?':
                req->set_uri(buf->get_pos(), p);
                buf->set_pos(p);
                state = PARSE_ABS_PATH_QUESTION;
                break;
            case '%':
                req->set_quoted(true);
                break;
            case CR:
            case LF:
                return PARSE_ERROR;
            }

            break;

        case PARSE_ABS_PATH_QUESTION:
            switch (ch) {
            case ' ':
                state = PARSE_VERSION_H;
                break;
            case '#':
                state = PARSE_ABS_PATH_FRAGMENT;
                break;
            case CR:
            case LF:
                return PARSE_ERROR;
            }

            buf->set_pos(p);
            state = PARSE_ABS_PATH_ARGS;
            break;

        case PARSE_ABS_PATH_ARGS:
            switch (ch) {
            case ' ':
                req->set_args(buf->get_pos(), p);
                buf->set_pos(p);
                state = PARSE_VERSION_H;
                break;
            case '#':
                req->set_args(buf->get_pos(), p);
                buf->set_pos(p);
                state = PARSE_ABS_PATH_FRAGMENT;
                break;
            case CR:
            case LF:
                return PARSE_ERROR;
            }

            break;

        case PARSE_ABS_PATH_FRAGMENT:
            switch (ch) {
            case ' ':
                state = PARSE_VERSION_H;
                buf->set_pos(p);
                break;
            case CR:
            case LF:
                return PARSE_ERROR;
            }

            break;

        case PARSE_VERSION_H:
            if (ch == 'H') {
                state = PARSE_VERSION_HT;
                break;
            }

            if (ch == ' ') {
                break;
            }

            return PARSE_ERROR;

        case PARSE_VERSION_HT:
            if (ch == 'T') {
                state = PARSE_VERSION_HTT;
                break;
            }

            return PARSE_ERROR;

        case PARSE_VERSION_HTT:
            if (ch == 'T') {
                state = PARSE_VERSION_HTTP;
                break;
            }

            return PARSE_ERROR;

        case PARSE_VERSION_HTTP:
            if (ch == 'P') {
                state = PARSE_VERSION_HTTP_SLASH;
                break;
            }

            return PARSE_ERROR;

        case PARSE_VERSION_HTTP_SLASH:
            if (ch == '/') {
                state = PARSE_VERSION_HTTP_SLASH_NUMBER;
                break;
            }

            return PARSE_ERROR;

        case PARSE_VERSION_HTTP_SLASH_NUMBER:
            if (ch >= '0' && ch <= '9') {
                buf->set_pos(p);
                state = PARSE_VERSION_HTTP_SLASH_VERSION;
                break;
            }

            return PARSE_ERROR;

        case PARSE_VERSION_HTTP_SLASH_VERSION:
            if ((ch >= '0' && ch <= '9') || ch == '.') {
                break;
            }

            switch (ch) {
            case ' ':
                req->set_version(buf->get_pos(), p);
                state = PARSE_LAST_CR;
                break;
            case CR:
                req->set_version(buf->get_pos(), p);
                state = PARSE_LAST_CR_LF;
                break;
            case LF:
                req->set_version(buf->get_pos(), p);
                state = PARSE_DONE;
                break;
            }

            return PARSE_ERROR;

        case PARSE_LAST_CR:
            switch (ch) {
            case ' ':
                break;
            case CR:
                state = PARSE_LAST_CR_LF;
                break;
            case LF:
                state = PARSE_DONE;
                break;
            }

            return PARSE_ERROR;

        case PARSE_LAST_CR_LF:
            if (ch == LF) {
                req->set_parse_state(PARSE_START);
                return PARSE_SUCCESS;
            }

            return PARSE_ERROR;
        }
    }

    req->set_parse_state(state);
    req->set_buf_offset(last - buf->get_pos());
    return PARSE_AGAIN;
}

int http_parse_request_headers(HttpRequest* req) {
    static uint8_t valid[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 0
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 16
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,     // 32
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,     // 48
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 64
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,     // 80
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 96
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,     // 112
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 128
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 144
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 160
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 176
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 192
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 208
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 224
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 240
    };

    int state = req->get_parse_state();
    Buffer *buf = req->get_recv_buf();
    char *last = buf->get_last();
    char ch;

    for (char *p = buf->get_pos() + req->get_buf_offset(); p < last; ++p) {
        ch = *p;

        // allow underscores
        switch (state) {
        case PARSE_START:
            if (valid[static_cast<uint8_t>(ch)]) {
                buf->set_pos(p);
                state = PARSE_HEADERS_NAME;
                break;
            }

            switch (ch) {
            case CR:
                state = PARSE_LAST_CR_LF_CR_LF;
                break;
            case LF:
                state = PARSE_DONE;
                break;
            default:
                return PARSE_ERROR;
            }

            break;

        case PARSE_HEADERS_NAME:
            if (valid[static_cast<uint8_t>(ch)]) {
                break;
            }

            switch (ch) {
            case ':':
                req->set_headers_in_name(buf->get_pos(), p);
                buf->set_pos(p);
                state = PARSE_HEADERS_COLON;
                break;
            default:
                return PARSE_ERROR;
            }

            break;

        case PARSE_HEADERS_COLON:
            if (valid[static_cast<uint8_t>(ch)]) {
                buf->set_pos(p);
                state = PARSE_HEADERS_VALUE;
                break;
            }

            switch (ch) {
            case ' ':
                break;
            default:
                return PARSE_ERROR;
            }

            break;

        case PARSE_HEADERS_VALUE:
            if (valid[static_cast<uint8_t>(ch)]) {
                break;
            }

            switch (ch) {
            case ' ':
                req->set_headers_in_value(buf->get_pos(), p);
                buf->set_pos(p);
                state = PARSE_LAST_CR;
                break;
            case CR:
                req->set_headers_in_value(buf->get_pos(), p);
                buf->set_pos(p);
                state = PARSE_LAST_CR_LF;
                break;
            case LF:
                req->set_headers_in_value(buf->get_pos(), p);
                buf->set_pos(p);
                state = PARSE_LAST_CR_LF_CR;
                break;
            default:
                return PARSE_ERROR;
            }

            break;

        case PARSE_LAST_CR:
            switch (ch) {
            case ' ':
                break;
            case CR:
                state = PARSE_LAST_CR_LF;
                break;
            case LF:
                state = PARSE_START;
                break;
            }

            return PARSE_ERROR;

        case PARSE_LAST_CR_LF:
            if (ch == LF) {
                state = PARSE_START;
                break;
            }

            return PARSE_ERROR;

        case PARSE_LAST_CR_LF_CR:
            if (ch == CR) {
                state = PARSE_LAST_CR_LF_CR_LF;
                break;
            }

            if (ch == LF) {
                return PARSE_SUCCESS;
            }

            return PARSE_ERROR;

        case PARSE_LAST_CR_LF_CR_LF:
            if (ch == LF) {
                return PARSE_SUCCESS;
            }

            return PARSE_ERROR;
        }
    }

    req->set_parse_state(state);
    req->set_buf_offset(last - buf->get_pos());
    return PARSE_AGAIN;
}

int http_parse_quoted(HttpRequest* req) {
    return PARSE_SUCCESS;
}

int http_parse_args(HttpRequest* req) {
    return PARSE_SUCCESS;
}

}

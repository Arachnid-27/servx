#include "http_parse.h"

#include "core.h"

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
    PARSE_ABSOLUTE_URI_PORT,
    PARSE_ABS_PATH,
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
            req->set_method(HTTP_METHOD_GET);
            break;
        }

        if (compare_method_4(p1, "HEAD")) {
            req->set_method(HTTP_METHOD_HEAD);
            break;
        }

        if (compare_method_4(p1, "POST")) {
            req->set_method(HTTP_METHOD_POST);
            break;
        }

        if (compare_method_4(p1, "PUT ")) {
            req->set_method(HTTP_METHOD_PUT);
            break;
        }

        break;
    case 6:
        if (compare_method_6(p1, "DELETE")) {
            req->set_method(HTTP_METHOD_DELETE);
        }

        break;
    case 7:
        if (compare_method_8(p1, "OPTIONS ")) {
            req->set_method(HTTP_METHOD_OPTIONS);
        }

        break;
    }
}

int http_parse_request_line(HttpRequest* req) {
    static const char* slash = "/";

    int state = req->get_parse_state();
    Buffer *buf = req->get_recv_buf();
    char *start = buf->get_pos();
    char *last = buf->get_last();
    char c, ch;

    for (char *p = start + req->get_buf_offset(); p < last; ++p) {
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

            return SERVX_ERROR;

        case PARSE_METHOD:
            if (ch >= 'A' && ch <= 'Z') {
                break;
            }

            if (ch == ' ') {
                http_parse_method(req, start, p);
                req->set_method(std::string(start, p));
                start = p + 1;
                state = PARSE_BEFORE_URI;
                break;
            }

            return SERVX_ERROR;

        case PARSE_BEFORE_URI:
            if (ch == '/') {
                start = p;
                state = PARSE_ABS_PATH;
                break;
            }

            c = LOWER_CASE(ch);
            if (c >= 'a' && c <= 'z') {
                state = PARSE_ABSOLUTE_URI_SCHEMA;
                break;
            }

            if (ch == ' ') {
                start = p + 1;
                break;
            }

            return SERVX_ERROR;

        case PARSE_ABSOLUTE_URI_SCHEMA:
            c = LOWER_CASE(ch);
            if (c >= 'a' && c <= 'z') {
                break;
            }

            if (ch == ':') {
                req->set_schema(std::string(start, p));
                start = p + 1;
                state = PARSE_ABSOLUTE_URI_SCHEMA_SLASH;
                break;
            }

            return SERVX_ERROR;

        case PARSE_ABSOLUTE_URI_SCHEMA_SLASH:
            if (ch == '/') {
                start = p + 1;
                state = PARSE_ABSOLUTE_URI_SCHEMA_SLASH_2;
                break;
            }

            return SERVX_ERROR;

        case PARSE_ABSOLUTE_URI_SCHEMA_SLASH_2:
            if (ch == '/') {
                start = p + 1;
                state = PARSE_ABSOLUTE_URI_HOST;
                break;
            }

            return SERVX_ERROR;

        case PARSE_ABSOLUTE_URI_HOST:   // rfc 3986 3.2.2
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
                req->set_host(std::string(start, p));
                start = p + 1;
                state = PARSE_ABSOLUTE_URI_PORT;
                break;
            case '/':
                req->set_host(std::string(start, p));
                start = p;
                state = PARSE_ABS_PATH;
                break;
            case ' ':
                req->set_host(std::string(start, p));
                req->set_uri(std::string(slash, slash + 1));
                start = p + 1;
                state = PARSE_VERSION_H;
                break;
            default:
                return SERVX_ERROR;
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

            return SERVX_ERROR;

        case PARSE_ABSOLUTE_URI_PORT:
            if (ch >= '0' && ch <= '9') {
                break;
            }

            switch (ch) {
            case '/':
                req->set_port(std::string(start, p));
                start = p;
                state = PARSE_ABS_PATH;
                break;
            case ' ':
                req->set_port(std::string(start, p));
                req->set_uri(std::string(slash, slash + 1));
                start = p + 1;
                state = PARSE_VERSION_H;
                break;
            default:
                return SERVX_ERROR;
            }

            break;

        case PARSE_ABS_PATH:
            switch (ch) {
            case ' ':
                req->set_uri(std::string(start, p));
                start = p + 1;
                state = PARSE_VERSION_H;
                break;
            case '#':
                req->set_uri(std::string(start, p));
                start = p + 1;
                state = PARSE_ABS_PATH_FRAGMENT;
                break;
            case '?':
                req->set_uri(std::string(start, p));
                start = p + 1;
                state = PARSE_ABS_PATH_ARGS;
                break;
            case '%':
                req->set_quoted(true);
                break;
            case CR:
            case LF:
                return SERVX_ERROR;
            }

            break;

        case PARSE_ABS_PATH_ARGS:
            switch (ch) {
            case ' ':
                req->set_args(std::string(start, p));
                start = p + 1;
                state = PARSE_VERSION_H;
                break;
            case '#':
                req->set_args(std::string(start, p));
                start = p + 1;
                state = PARSE_ABS_PATH_FRAGMENT;
                break;
            case CR:
            case LF:
                return SERVX_ERROR;
            }

            break;

        case PARSE_ABS_PATH_FRAGMENT:
            start = p + 1;

            switch (ch) {
            case ' ':
                state = PARSE_VERSION_H;
                break;
            case CR:
            case LF:
                return SERVX_ERROR;
            }

            break;

        case PARSE_VERSION_H:
            switch (ch) {
            case 'H':
                state = PARSE_VERSION_HT;
                start = p + 1;
                break;
            case ' ':
                start = p + 1;
                break;
            default:
                return SERVX_ERROR;
            }

            break;

        case PARSE_VERSION_HT:
            if (ch == 'T') {
                state = PARSE_VERSION_HTT;
                start = p + 1;
                break;
            }

            return SERVX_ERROR;

        case PARSE_VERSION_HTT:
            if (ch == 'T') {
                state = PARSE_VERSION_HTTP;
                start = p + 1;
                break;
            }

            return SERVX_ERROR;

        case PARSE_VERSION_HTTP:
            if (ch == 'P') {
                state = PARSE_VERSION_HTTP_SLASH;
                start = p + 1;
                break;
            }

            return SERVX_ERROR;

        case PARSE_VERSION_HTTP_SLASH:
            if (ch == '/') {
                state = PARSE_VERSION_HTTP_SLASH_NUMBER;
                start = p + 1;
                break;
            }

            return SERVX_ERROR;

        case PARSE_VERSION_HTTP_SLASH_NUMBER:
            if (ch >= '0' && ch <= '9') {
                state = PARSE_VERSION_HTTP_SLASH_VERSION;
                break;
            }

            return SERVX_ERROR;

        case PARSE_VERSION_HTTP_SLASH_VERSION:
            if ((ch >= '0' && ch <= '9') || ch == '.') {
                break;
            }

            switch (ch) {
            case ' ':
                req->set_version(std::string(start, p));
                start = p + 1;
                state = PARSE_LAST_CR;
                break;
            case CR:
                req->set_version(std::string(start, p));
                start = p + 1;
                state = PARSE_LAST_CR_LF;
                break;
            case LF:
                req->set_version(std::string(start, p));
                buf->set_pos(p + 1);
                req->set_parse_state(PARSE_START);
                req->set_buf_offset(0);
                return SERVX_OK;
            default:
                return SERVX_ERROR;
            }

            break;

        case PARSE_LAST_CR:
            switch (ch) {
            case ' ':
                start = p + 1;
                break;
            case CR:
                state = PARSE_LAST_CR_LF;
                start = p + 1;
                break;
            case LF:
                buf->set_pos(p + 1);
                req->set_buf_offset(0);
                req->set_parse_state(PARSE_START);
                return SERVX_OK;
            default:
                return SERVX_ERROR;
            }

            break;

        case PARSE_LAST_CR_LF:
            if (ch == LF) {
                buf->set_pos(p + 1);
                req->set_buf_offset(0);
                req->set_parse_state(PARSE_START);
                return SERVX_OK;
            }

            return SERVX_ERROR;
        }
    }

    buf->set_pos(start);
    req->set_parse_state(state);
    req->set_buf_offset(last - start);
    return SERVX_AGAIN;
}

int http_parse_request_headers(HttpRequest* req) {
    // allow underscores
    static uint8_t lowercase[] = {
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 , '-',  0 ,  0 ,
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 , 'a', 'b', 'c', 'd', 'e', 'f', 'g',
        'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
        'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
        'x', 'y', 'z',  0 ,  0 ,  0 ,  0 ,  0 ,
        '_', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
        'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
        'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
        'x', 'y', 'z',  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
    };

    int state = req->get_parse_state();
    Buffer *buf = req->get_recv_buf();
    char *start = buf->get_pos();
    char *last = buf->get_last();
    char ch;

    for (char *p = start + req->get_buf_offset(); p < last; ++p) {
        ch = *p;

        switch (state) {
        case PARSE_START:
            switch (ch) {
            case CR:
                state = PARSE_LAST_CR_LF_CR_LF;
                break;
            case LF:
                req->set_parse_state(PARSE_DONE);
                req->set_buf_offset(0);
                buf->set_pos(p + 1);
                return SERVX_OK;
            default:
                ch = lowercase[static_cast<uint8_t>(ch)];
                if (ch) {
                    *p = ch;
                    state = PARSE_HEADERS_NAME;
                    break;
                }
                return SERVX_ERROR;
            }

            break;

        case PARSE_HEADERS_NAME:
            switch (ch) {
            case ':':
                req->set_headers_name(std::string(start, p));
                start = p + 1;
                state = PARSE_HEADERS_COLON;
                break;
            default:
                ch = lowercase[static_cast<uint8_t>(ch)];
                if (ch) {
                    *p = ch;
                    break;
                }
                return SERVX_ERROR;
            }

            break;

        case PARSE_HEADERS_COLON:
            switch (ch) {
            case ' ':
                break;
            case '\0':
                return SERVX_ERROR;
            default:
                start = p;
                state = PARSE_HEADERS_VALUE;
                break;
            }

            break;

        case PARSE_HEADERS_VALUE:
            switch (ch) {
            case CR:
                req->set_headers_value(std::string(start, p));
                start = p + 1;
                state = PARSE_LAST_CR_LF;
                break;
            case LF:
                req->set_headers_value(std::string(start, p));
                start = p + 1;
                state = PARSE_LAST_CR_LF_CR;
                break;
            case '\0':
                return SERVX_ERROR;
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
                start = p + 1;
                state = PARSE_START;
                break;
            }

            return SERVX_ERROR;

        case PARSE_LAST_CR_LF:
            if (ch == LF) {
                start = p + 1;
                state = PARSE_START;
                break;
            }

            return SERVX_ERROR;

        case PARSE_LAST_CR_LF_CR:
            switch (ch) {
            case CR:
                state = PARSE_LAST_CR_LF_CR_LF;
                break;
            case LF:
                return SERVX_OK;
            default:
                return SERVX_ERROR;
            }

            break;

        case PARSE_LAST_CR_LF_CR_LF:
            if (ch == LF) {
                req->set_parse_state(PARSE_DONE);
                req->set_buf_offset(0);
                buf->set_pos(p + 1);
                return SERVX_OK;
            }

            return SERVX_ERROR;
        }
    }

    buf->set_pos(start);
    req->set_parse_state(state);
    req->set_buf_offset(last - start);
    return SERVX_AGAIN;
}

int http_parse_quoted(HttpRequest* req) {
    return SERVX_OK;
}

int http_parse_args(HttpRequest* req) {
    return SERVX_OK;
}

}

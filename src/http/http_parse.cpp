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
    PARSE_ABSOLUTE_URI_PORT,
    PARSE_ABS_PATH,
    PARSE_HTTP_H,
};

int http_parse_request_line(HttpRequest* req) {
    static const char* slash = "/";

    int state = req->get_parse_state();
    Buffer *buf = req->get_recv_buf();
    char *last = buf->get_last();
    char c, ch;

    for (char *p = buf->get_pos(); p < last; ++p) {
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

            return PARSE_INVALID_METHOD;

        case PARSE_METHOD:
            if (ch >= 'A' && ch <= 'Z') {
                break;
            }

            if (ch == ' ') {
                char *pos = buf->get_pos();
                switch (pos - p) {
                case 3:
                case 4:
                    if (compare_method_4(pos, "GET ")) {
                        req->set_method(METHOD_GET);
                        break;
                    }

                    if (compare_method_4(pos, "HEAD")) {
                        req->set_method(METHOD_HEAD);
                        break;
                    }

                    if (compare_method_4(pos, "POST")) {
                        req->set_method(METHOD_POST);
                        break;
                    }

                    if (compare_method_4(pos, "PUT ")) {
                        req->set_method(METHOD_PUT);
                        break;
                    }

                    break;
                case 6:
                    if (compare_method_6(pos, "DELETE")) {
                        req->set_method(METHOD_DELETE);
                        break;
                    }

                    break;
                case 7:
                    if (compare_method_8(pos, "OPTIONS ")) {
                        req->set_method(METHOD_OPTIONS);
                        break;
                    }

                    break;
                }

                state = PARSE_BEFORE_URI;
                req->set_method(pos, p);
                buf->set_pos(p);
                break;
            }

            return PARSE_INVALID_METHOD;

        case PARSE_BEFORE_URI:
            if (ch == '/') {
                state = PARSE_ABS_PATH;
                break;
            }

            c = LOWER_CASE(ch);
            if (c >= 'a' && c <= 'z') {
                state = PARSE_ABSOLUTE_URI_SCHEMA;
                break;
            }

            if (ch == ' ') {
                break;
            }

            return PARSE_INVALID_REQUEST;

        case PARSE_ABSOLUTE_URI_SCHEMA:
            c = LOWER_CASE(ch);
            if (c >= 'a' && c <= 'z') {
                break;
            }

            if (ch == ':') {
                req->set_schema(buf->get_pos(), p);
                buf->set_pos(p);
                state = PARSE_ABSOLUTE_URI_SCHEMA_SLASH;
                break;
            }

            return PARSE_INVALID_REQUEST;

        case PARSE_ABSOLUTE_URI_SCHEMA_SLASH:
            if (ch == '/') {
                state = PARSE_ABSOLUTE_URI_SCHEMA_SLASH_2;
                break;
            }

            return PARSE_INVALID_REQUEST;

        case PARSE_ABSOLUTE_URI_SCHEMA_SLASH_2:
            if (ch == '/') {
                buf->set_pos(p);
                state = PARSE_ABSOLUTE_URI_HOST;
                break;
            }

            return PARSE_INVALID_REQUEST;

        case PARSE_ABSOLUTE_URI_HOST:   // rfc 3986 3.2.2
            if (ch == '[') {
                state = PARSE_ABSOLUTE_URI_HOST_IPV6;
                break;
            }

            state = PARSE_ABSOLUTE_URI_HOST_IPV4_OR_REG_NAME;

            // fall through

        case PARSE_ABSOLUTE_URI_HOST_IPV4_OR_REG_NAME:
            if (ch >= '0' && ch <= '9') {
                break;
            }

            c = LOWER_CASE(ch);
            if (c >= 'a' && c <= 'z') {
                break;
            }

            if (ch == '.' || ch == '-') {
                break;
            }

            // fall through

        case PARSE_ABSOLUTE_URI_HOST_END:
            switch (ch) {
            case ':':
                req->set_host(buf->get_pos(), p);
                buf->set_pos(p);
                state = PARSE_ABSOLUTE_URI_PORT;
                break;
            case '/':
                req->set_host(buf->get_pos(), p);
                buf->set_pos(p);
                state = PARSE_ABS_PATH;
                break;
            case ' ':
                req->set_host(buf->get_pos(), p);
                req->set_uri(slash, slash + 1);
                buf->set_pos(p);
                state = PARSE_HTTP_H;
                break;
            default:
                return PARSE_INVALID_REQUEST;
            }

            break;

        case PARSE_ABSOLUTE_URI_HOST_IPV6:
            if (ch >= '0' && ch <= '9') {
                break;
            }

            c = LOWER_CASE(ch);
            if (c >= 'a' && c <= 'f') {
                break;
            }

            if (c == ':' || c == '.') {
                break;
            }

            if (c == ']') {
                state = PARSE_ABSOLUTE_URI_HOST_END;
                break;
            }

            return PARSE_INVALID_REQUEST;

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
                buf->set_pos(p);
                state = PARSE_HTTP_H;
                break;
            default:
                return PARSE_INVALID_REQUEST;
            }

            break;

        case PARSE_ABS_PATH:
            break;

        }
    }

    req->set_parse_state(state);
    return PARSE_AGAIN;
}

}

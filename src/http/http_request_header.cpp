#include "http_request_header.h"

#include "http_request.h"

namespace servx {

int HttpRequestHeader::parse_request_line() {
    static const char* slash = "/";

    char *start = buffer->get_pos();
    char *last = buffer->get_last();
    char c, ch;

    for (char *p = start; p < last; ++p) {
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
                // http_parse_method(req, start, p);
                method = std::string(start, p);
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
                schema = std::string(start, p);
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
                host = std::string(start, p);
                start = p + 1;
                state = PARSE_ABSOLUTE_URI_PORT;
                break;
            case '/':
                host = std::string(start, p);
                start = p;
                state = PARSE_ABS_PATH;
                break;
            case ' ':
                host = std::string(start, p);
                uri = std::string(slash, slash + 1);
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
                port = std::string(start, p);
                start = p;
                state = PARSE_ABS_PATH;
                break;
            case ' ':
                port = std::string(start, p);
                uri = std::string(slash, slash + 1);
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
                uri = std::string(start, p);
                start = p + 1;
                state = PARSE_VERSION_H;
                break;
            case '#':
                uri = std::string(start, p);
                start = p + 1;
                state = PARSE_ABS_PATH_FRAGMENT;
                break;
            case '?':
                uri = std::string(start, p);
                start = p + 1;
                state = PARSE_ABS_PATH_ARGS;
                break;
            case '%':
                quoted = true;
                break;
            case CR:
            case LF:
                return SERVX_ERROR;
            }

            break;

        case PARSE_ABS_PATH_ARGS:
            switch (ch) {
            case ' ':
                args = std::string(start, p);
                start = p + 1;
                state = PARSE_VERSION_H;
                break;
            case '#':
                args = std::string(start, p);
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
                version = std::string(start, p);
                start = p + 1;
                state = PARSE_LAST_CR;
                break;
            case CR:
                version = std::string(start, p);
                start = p + 1;
                state = PARSE_LAST_CR_LF;
                break;
            case LF:
                version = std::string(start, p);
                buffer->set_pos(p + 1);
                state = PARSE_START;
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
                buffer->set_pos(p + 1);
                state = PARSE_START;
                return SERVX_OK;
            default:
                return SERVX_ERROR;
            }

            break;

        case PARSE_LAST_CR_LF:
            if (ch == LF) {
                buffer->set_pos(p + 1);
                state = PARSE_START;
                return SERVX_OK;
            }

            return SERVX_ERROR;
        }
    }

    buffer->set_pos(start);
    return SERVX_AGAIN;
}

int HttpRequestHeader::parse_request_headers() {
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

    char *start = buffer->get_pos();
    char *last = buffer->get_last();
    char ch;

    for (char *p = start; p < last; ++p) {
        ch = *p;

        switch (state) {
        case PARSE_START:
            switch (ch) {
            case CR:
                state = PARSE_LAST_CR_LF_CR_LF;
                break;
            case LF:
                temp = "";
                state = PARSE_DONE;
                buffer->set_pos(p + 1);
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
                temp = std::string(start, p);
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
                headers.emplace(std::move(temp), std::string(start, p));
                start = p + 1;
                state = PARSE_LAST_CR_LF;
                break;
            case LF:
                headers.emplace(std::move(temp), std::string(start, p));
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
                temp = "";
                state = PARSE_DONE;
                buffer->set_pos(p + 1);
                return SERVX_OK;
            }

            return SERVX_ERROR;
        }
    }

    buffer->set_pos(start);
    return SERVX_AGAIN;
}

HttpMethod HttpRequestHeader::parse_request_method() {
    const char *p = method.data();
    switch (method.length()) {
    case 3:
    case 4:
        if (CMP_METHOD_4(p, "GET")) {   // null-terminated
            return HTTP_METHOD_GET;
        }

        if (CMP_METHOD_4(p, "HEAD")) {
            return HTTP_METHOD_HEAD;
        }

        if (CMP_METHOD_4(p, "POST")) {
            return HTTP_METHOD_POST;
        }

        if (CMP_METHOD_4(p, "PUT")) {   // null-terminated
            return HTTP_METHOD_PUT;
        }

        break;
    case 6:
        if (CMP_METHOD_6(p, "DELETE")) {
            return HTTP_METHOD_DELETE;
        }

        break;
    case 7:
        if (CMP_METHOD_8(p, "OPTIONS")) {   // null-terminated
            return HTTP_METHOD_OPTIONS;
        }

        break;
    }

    return HTTP_METHOD_UNKONWN;
}

}

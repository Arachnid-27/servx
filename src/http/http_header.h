#ifndef _HTTP_HEADER_H_
#define _HTTP_HEADER_H_

#include <string>
#include <unordered_map>

#include "buffer.h"
#include "http.h"

namespace servx {

class HttpHeader {
public:
    HttpHeader(Buffer *buf): buffer(buf), state(PARSE_START) {}

    HttpHeader(const HttpHeader&) = delete;
    HttpHeader(HttpHeader&&) = delete;
    HttpHeader& operator=(const HttpHeader&) = delete;
    HttpHeader& operator=(HttpHeader&&) = delete;

    ~HttpHeader() = default;

    int parse_headers();

    const std::string& get_header(const std::string key) const {
        auto iter = headers.find(key);
        return iter == headers.end() ? temp : iter->second;
    }

    const std::unordered_map<std::string, std::string>&
    get_headers() const { return headers; }

protected:
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

        PARSE_STATUS_CODE_1,
        PARSE_STATUS_CODE_2,
        PARSE_STATUS_CODE_3,
        PARSE_STATUS_BEFORE_DESCRIPTION,
        PARSE_STATUS_DESCRIPTION,

        PARSE_LINE_DONE,

        PARSE_HEADERS_NAME,
        PARSE_HEADERS_COLON,
        PARSE_HEADERS_VALUE,

        PARSE_LAST_CR,
        PARSE_LAST_CR_LF,
        PARSE_LAST_CR_LF_CR,
        PARSE_LAST_CR_LF_CR_LF,

        PARSE_DONE
    };

    Buffer* buffer;

    int state;

    std::string temp;
    std::unordered_map<std::string, std::string> headers;
};

class HttpRequestHeader: public HttpHeader {
    friend class HttpRequest;
public:
    HttpRequestHeader(Buffer *buf): HttpHeader(buf) {}

    ~HttpRequestHeader() = default;

    int parse_request_line();

    HttpMethod parse_request_method();

    const std::string& get_method() const { return method; }
    const std::string& get_uri() const { return uri; }
    const std::string& get_args() const { return args; }

private:
    char LOWER_CASE(char ch) const { return (ch | 0x20); }

    int CMP_METHOD_4(const char* p1, const char* p2) {
        return *reinterpret_cast<const uint32_t*>(p1) ==
               *reinterpret_cast<const uint32_t*>(p2);
    }

    int CMP_METHOD_6(const char* p1, const char* p2) {
        return *reinterpret_cast<const uint32_t*>(p1) ==
               *reinterpret_cast<const uint32_t*>(p2) &&
               *reinterpret_cast<const uint16_t*>(p1 + 4) ==
               *reinterpret_cast<const uint16_t*>(p2 + 4);
    }

    int CMP_METHOD_8(const char* p1, const char* p2) {
        return *reinterpret_cast<const uint32_t*>(p1) ==
               *reinterpret_cast<const uint32_t*>(p2) &&
               *reinterpret_cast<const uint32_t*>(p1 + 4) ==
               *reinterpret_cast<const uint32_t*>(p2 + 4);
    }

    bool quoted;

    std::string method;
    std::string schema;
    std::string host;
    std::string port;
    std::string uri;
    std::string args;
    std::string version;
};

class HttpResponseHeader: public HttpHeader {
public:
    HttpResponseHeader(Buffer *buf): HttpHeader(buf) {}

    ~HttpResponseHeader() = default;

    int parse_response_line();

private:
    std::string version;
    std::string status;
    std::string description;
};

}

#endif

#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include <unordered_map>

#include "server.h"

#define set_req_attr(attr)           \
    attr = std::string(p1, p2);

namespace servx {

enum HttpMethod {
    METHOD_UNKONWN,
    METHOD_GET,
    METHOD_HEAD,
    METHOD_POST,
    METHOD_PUT,
    METHOD_DELETE,
    METHOD_OPTIONS
};

enum HttpStateCode {
    HTTP_CONTINUE = 100,

    HTTP_OK = 200,

    HTTP_MOVED_PERMANENTLY = 301,
    HTTP_MOVED_TEMPORARILY = 301,

    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_NOT_ALLOWED = 405,
    HTTP_REQUEST_TIME_OUT = 408,
    HTTP_CONFLICT = 409,
    HTTP_LENGTH_REQUIRED = 411,
    HTTP_PRECONDITION_FAILED = 412,
    HTTP_REQUEST_ENTITY_TOO_LARGE = 413,
    HTTP_REQUEST_URI_TOO_LARGE = 414,

    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVAILABLE = 503
};

class HttpRequest;

using http_req_handler_t = std::function<void(HttpRequest*)>;

class HttpRequest {
public:
    HttpRequest(Buffer* buf);

    HttpRequest(const HttpRequest&) = delete;
    HttpRequest(HttpRequest&&) = delete;
    HttpRequest& operator=(const HttpRequest&) = delete;
    HttpRequest& operator=(HttpRequest&&) = delete;

    ~HttpRequest() = default;

    HttpMethod get_method() const { return http_method; }
    void set_method(HttpMethod hm) { http_method = hm; }

    void set_method(const char* p1, const char* p2) { set_req_attr(method); }
    void set_schema(const char* p1, const char* p2) { set_req_attr(schema); }
    void set_host(const char* p1, const char* p2) { set_req_attr(host); }
    void set_port(const char* p1, const char* p2) { set_req_attr(port); }
    void set_uri(const char* p1, const char* p2) { set_req_attr(uri); }

    void handle_read() { read_handler(this); }
    void set_read_handler(const http_req_handler_t& h) { read_handler = h; }

    void handle_write() { write_handler(this); }
    void set_write_handler(const http_req_handler_t& h) { write_handler = h; }

    int get_parse_state() const { return parse_state; }
    void set_parse_state(int state) { parse_state = state; }

    Buffer* get_recv_buf() const { return recv_buf.get(); }

    void close(int state) {}

private:
    HttpMethod http_method;
    int parse_state;
    // void** ctx;
    http_req_handler_t read_handler;
    http_req_handler_t write_handler;
    std::unique_ptr<Buffer> recv_buf;
    std::unordered_map<std::string, std::string> headers_in;
    std::unordered_map<std::string, std::string> headers_out;

    std::string method;
    std::string schema;
    std::string host;
    std::string port;
    std::string uri;
};

class HttpConnection: public ConnectionContext {
public:
    void set_server(Server* srv) { server = srv; }

    HttpRequest* get_request() const { return request.get(); }
    void set_request(HttpRequest* req);

private:
    Server *server;
    std::unique_ptr<HttpRequest> request;
};

inline void HttpConnection::set_request(HttpRequest* req) {
    request = std::unique_ptr<HttpRequest>(req);
}

void http_wait_request_handler(Event* ev);

void http_empty_handler(Event* ev);

void http_init_connection(Connection* conn);

void http_block_reading(HttpRequest* req);

void http_process_request_line(Event* ev);

}

#endif

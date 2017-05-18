#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include <unordered_map>

#include "connection.h"
#include "http_phase.h"
#include "http_response.h"
#include "server.h"

namespace servx {

enum HttpMethod {
    HTTP_METHOD_UNKONWN,
    HTTP_METHOD_GET,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_OPTIONS
};

class HttpRequest;
class HttpPhaseHandler;

using http_req_handler_t = std::function<void(HttpRequest*)>;

class HttpRequest {
public:
    HttpRequest(Connection* c);

    HttpRequest(const HttpRequest&) = delete;
    HttpRequest(HttpRequest&&) = delete;
    HttpRequest& operator=(const HttpRequest&) = delete;
    HttpRequest& operator=(HttpRequest&&) = delete;

    ~HttpRequest() = default;

    HttpMethod get_http_method() const { return http_method; }
    void set_method(HttpMethod hm) { http_method = hm; }

    void set_method(std::string&& s) { method = std::move(s); }
    void set_schema(std::string&& s) { schema = std::move(s); }
    void set_host(std::string&& s) { host = std::move(s); }
    void set_port(std::string&& s) { port = std::move(s); }
    void set_uri(std::string&& s) { uri = std::move(s); }
    void set_args(std::string&& s) { args = std::move(s); }
    void set_version(std::string&& s) { version = std::move(s); }

    const std::string& get_method() const { return method; }
    const std::string& get_schema() const { return schema; }
    const std::string& get_host() const { return host; }
    const std::string& get_port() const { return port; }
    const std::string& get_uri() const { return uri; }
    const std::string& get_args() const { return args; }
    const std::string& get_version() const { return version; }

    void handle_read() { read_handler(this); }
    void set_read_handler(const http_req_handler_t& h) { read_handler = h; }

    void handle_write() { write_handler(this); }
    void set_write_handler(const http_req_handler_t& h) { write_handler = h; }

    int get_parse_state() const { return parse_state; }
    void set_parse_state(int state) { parse_state = state; }

    Buffer* get_recv_buf() const { return conn->get_recv_buf(); }

    int get_buf_offset() const { return buf_offset; }
    void set_buf_offset(int n) { buf_offset = n; }

    Server* get_server() const { return server; }
    void set_server(Server* srv) { server = srv; }

    Location* get_location() const { return location; }
    void set_location(Location* loc) { location = loc; }

    bool is_quoted() const { return quoted; }
    void set_quoted(bool q) { quoted = q; }

    bool is_chunked() const { return chunked; }
    void set_chunked(bool c) { chunked = c; }

    bool is_keep_alive() const { return keep_alive; }
    void set_keep_alive(bool k) { keep_alive = k; }

    long get_content_length() const { return content_length; }
    void set_content_length(long n) { content_length = n; }

    bool is_lingering_close() const { return content_length > 0 || chunked; }

    void set_headers_name(std::string&& s) { name = std::move(s); }
    void set_headers_value(std::string&& s);

    std::string get_headers(const char* s) const;

    size_t get_phase_handler() const { return phase_handler; }
    void set_phase_handler(size_t index) { phase_handler = index; }
    void next_phase_handler() { ++phase_handler; }

    void get_content_handler(HttpPhaseHandler* h) { content_handler = h; }
    HttpPhaseHandler* get_content_handler() const { return content_handler; }

    bool is_discard_body() const { return discard_body; }
    bool discard_request_body();

    bool is_expect_tested() const { return expect_tested; }
    bool test_expect();

    HttpResponse* get_response() const { return response.get(); }

    int read_request_header();

    void finalize(int status) {}
    void close(int status) {}

private:
    Connection *conn;
    HttpMethod http_method;
    int parse_state;
    int buf_offset;
    long content_length;
    // void** ctx;
    http_req_handler_t read_handler;
    http_req_handler_t write_handler;
    std::unordered_map<std::string, std::string> headers;

    std::unique_ptr<HttpResponse> response;

    size_t phase_handler;
    HttpPhaseHandler *content_handler;

    Server *server;
    Location *location;

    uint32_t quoted:1;
    uint32_t chunked:1;
    uint32_t keep_alive:1;
    uint32_t discard_body:1;
    uint32_t expect_tested:1;

    std::string name;

    std::string method;
    std::string schema;
    std::string host;
    std::string port;
    std::string uri;
    std::string args;
    std::string version;
};

inline void HttpRequest::set_headers_value(std::string&& s) {
    headers.emplace(std::move(name), std::move(s));
}

class HttpConnection: public ConnectionContext {
public:
    HttpServers* get_servers() const { return servers; }
    void set_servers(HttpServers* srv) { servers = srv; }

    HttpRequest* get_request() const { return request.get(); }
    void set_request(HttpRequest* req);

private:
    HttpServers *servers;
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

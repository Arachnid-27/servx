#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include <unordered_map>

#include "connection.h"
#include "http.h"
#include "http_request_body.h"
#include "http_response.h"
#include "modules.h"
#include "server.h"

namespace servx {

struct HttpRequestContext {
};

class HttpRequestBody;

class HttpRequest {
public:
    using http_phase_handler_t = std::function<int(HttpRequest*)>;
    using http_req_handler_t = std::function<void(HttpRequest*)>;

    explicit HttpRequest(Connection* c);

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
    const std::string& get_uri() const { return uri; }

    void handle(Event* ev);

    void set_read_handler(const http_req_handler_t& h) { read_handler = h; }
    void set_write_handler(const http_req_handler_t& h) { write_handler = h; }

    int get_parse_state() const { return parse_state; }
    void set_parse_state(int state) { parse_state = state; }

    Connection* get_connection() const { return conn; }

    Buffer* get_recv_buf() const { return conn->get_recv_buf(); }

    Server* get_server() const { return server; }
    void set_server(Server* srv) { server = srv; }

    Location* get_location() const { return location; }
    void set_location(Location* loc) { location = loc; }

    void set_quoted(bool q) { quoted = q; }

    bool is_keep_alive() const { return keep_alive; }
    void set_keep_alive(bool k) { keep_alive = k; }

    void set_headers_name(std::string&& s) { name = std::move(s); }
    void set_headers_value(std::string&& s);

    std::string get_headers(const char* s) const;

    uint32_t get_phase() const { return phase; }
    void next_phase() { ++phase; phase_index = 0; }

    uint32_t get_phase_index() const { return phase_index; }
    void next_phase_index() { ++phase_index; }

    http_phase_handler_t get_content_handler() { return content_handler; }
    void get_content_handler(const http_phase_handler_t& h);

    HttpResponse* get_response() { return response.get(); }
    HttpRequestBody* get_request_body() { return body.get(); }

    template <typename T>
    typename T::request_context_t* get_context();

    template <typename T>
    void set_context(typename T::request_context_t* ctx);

    int read_headers();
    void process_line(Event* ev);
    void process_headers(Event* ev);

    void finalize(int rc);
    void close(int status);

private:
    Connection *conn;
    HttpMethod http_method;
    int parse_state;
    http_req_handler_t read_handler;
    http_req_handler_t write_handler;
    std::unordered_map<std::string, std::string> headers;

    std::unique_ptr<HttpRequestBody> body;
    std::unique_ptr<HttpResponse> response;

    std::unique_ptr<HttpRequestContext> context[NULL_MODULE];

    uint32_t phase;
    uint32_t phase_index;
    http_phase_handler_t content_handler;

    Server *server;
    Location *location;

    uint32_t quoted:1;
    uint32_t keep_alive:1;

    std::string name;

    std::string method;
    std::string schema;
    std::string host;
    std::string port;
    std::string uri;
    std::string args;
    std::string version;
};

inline void HttpRequest::get_content_handler(const http_phase_handler_t& h) {
    content_handler = h;
}

inline void HttpRequest::set_headers_value(std::string&& s) {
    headers.emplace(std::move(name), std::move(s));
}

template <typename T>
typename T::request_context_t* HttpRequest::get_context() {
    return static_cast<typename T::request_context_t*>(context[T::index]);
}

template <typename T>
void HttpRequest::set_context(typename T::request_context_t* ctx) {
    context[T::index] = std::unique_ptr<HttpRequestContext>(ctx);
}

void http_block_reading(HttpRequest* req);
void http_block_writing(HttpRequest* req);

}

#endif

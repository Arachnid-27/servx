#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include "http.h"
#include "http_request_body.h"
#include "http_request_header.h"
#include "http_response.h"
#include "modules.h"
#include "server.h"

namespace servx {

struct HttpRequestContext {
    virtual ~HttpRequestContext() {}
};

class HttpRequest {
public:
    using http_req_handler_t = std::function<void(HttpRequest*)>;

    explicit HttpRequest(Connection* c);

    HttpRequest(const HttpRequest&) = delete;
    HttpRequest(HttpRequest&&) = delete;
    HttpRequest& operator=(const HttpRequest&) = delete;
    HttpRequest& operator=(HttpRequest&&) = delete;

    ~HttpRequest() = default;

    void handle(Event* ev);

    void set_read_handler(const http_req_handler_t& h) { read_handler = h; }
    void set_write_handler(const http_req_handler_t& h) { write_handler = h; }

    Connection* get_connection() const { return conn; }

    Server* get_server() const { return server; }
    void set_server(Server* srv) { server = srv; }

    Location* get_location() const { return location; }
    void set_location(Location* loc) { location = loc; }

    bool is_keep_alive() const { return keep_alive; }
    void set_keep_alive(bool k) { keep_alive = k; }

    uint32_t get_phase() const { return phase; }
    void next_phase() { ++phase; phase_index = 0; }

    uint32_t get_phase_index() const { return phase_index; }
    void next_phase_index() { ++phase_index; }

    HttpMethod get_http_method() const { return http_method; }

    HttpRequestHeader* get_request_header() { return &header; }
    HttpRequestBody* get_request_body() { return &body; }
    HttpResponse* get_response() { return &response; }

    template <typename T>
    typename T::request_context_t* get_context() {
        return static_cast<typename T::request_context_t*>(
            context[T::index].get());
    }

    template <typename T>
    void set_context(HttpRequestContext* ctx) {
        context[T::index] = std::unique_ptr<HttpRequestContext>(ctx);
    }

    int read_headers();
    void process_line(Event* ev);
    void process_headers(Event* ev);

    void finalize(int rc);
    void close(int status);

private:
    Connection *conn;
    http_req_handler_t read_handler;
    http_req_handler_t write_handler;

    HttpMethod http_method;

    HttpRequestHeader header;
    HttpRequestBody body;
    HttpResponse response;

    std::unique_ptr<HttpRequestContext> context[NULL_MODULE];

    uint32_t phase;
    uint32_t phase_index;

    Server *server;
    Location *location;

    bool keep_alive;
};

void http_block_reading(HttpRequest* req);
void http_block_writing(HttpRequest* req);

}

#endif

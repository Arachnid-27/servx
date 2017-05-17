#ifndef _HTTP_PHASE_H_
#define _HTTP_PHASE_H_

#include <functional>
#include <vector>

#include "http_request.h"

namespace servx {

enum HttpPhase {
    HTTP_POST_READ_PHASE,
//  HTTP_REWRITE_PHASE
    HTTP_FIND_CONFIG_PHASE,
//  HTTP_ACCESS_PHASE,
    HTTP_CONTENT_PHASE,
    HTTP_LOG_PHASE
};

enum HttpPhaseResultCode {
    HTTP_NEXT_PHASE,
    HTTP_NEXT_HANDLER,
    HTTP_PHASE_AGAIN,
    HTTP_PHASE_SUCCESS,
    HTTP_PHASE_ERROR,

    HTTP_CHECKER_AGAIN,
    HTTP_CHECKER_OK
};

class HttpRequest;

class HttpPhaseHandler {
public:
    HttpPhaseHandler(
        const std::function<int(HttpRequest*)>& h,
        const std::function<int(HttpRequest*, HttpPhaseHandler*)>& c,
        size_t n)
        : handler(h), checker(c), next(n) {}

    HttpPhaseHandler(const HttpPhaseHandler&) = delete;
    HttpPhaseHandler(HttpPhaseHandler&&) = delete;
    HttpPhaseHandler& operator=(const HttpPhaseHandler&) = delete;
    HttpPhaseHandler& operator=(HttpPhaseHandler&&) = delete;

    ~HttpPhaseHandler() = default;

    int check(HttpRequest* req) { return checker(req, this); }

    int handle(HttpRequest* req) { return handler(req); }

    size_t get_next() const { return next; }

private:
    std::function<int(HttpRequest*)> handler;
    std::function<int(HttpRequest*, HttpPhaseHandler*)> checker;
    size_t next;
};

class HttpPhaseRunner {
public:
    HttpPhaseRunner(const HttpPhaseRunner&) = delete;
    HttpPhaseRunner(HttpPhaseRunner&&) = delete;
    HttpPhaseRunner& operator=(const HttpPhaseRunner&) = delete;
    HttpPhaseRunner& operator=(HttpPhaseRunner&&) = delete;

    ~HttpPhaseRunner() = default;

    void register_handler(HttpPhase phase,
        const std::function<int(HttpRequest*)>& h);

    void init();

    void run(HttpRequest* req);

    static HttpPhaseRunner* instance() { return runner; }

private:
    HttpPhaseRunner() = default;

    int generic_phase_checker(HttpRequest* req, HttpPhaseHandler* ph);
//  int rewrite_phase_checker(HttpRequest* req, HttpPhaseHandler* ph);
    int find_config_phase_checker(HttpRequest* req, HttpPhaseHandler* ph);
//  int access_phase_checker(HttpRequest* req, HttpPhaseHandler* ph);
    int content_phase_checker(HttpRequest* req, HttpPhaseHandler* ph);

    std::vector<HttpPhaseHandler> phase_handlers;
    std::vector<
        std::function<int(HttpRequest*)>> handlers[HTTP_LOG_PHASE + 1];

    static HttpPhaseRunner* runner;
};

inline void HttpPhaseRunner::register_handler(
    HttpPhase phase, const std::function<int(HttpRequest*)>& h) {
    handlers[phase].push_back(h);
}

}

#endif

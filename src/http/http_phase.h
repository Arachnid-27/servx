#ifndef _HTTP_PHASE_H_
#define _HTTP_PHASE_H_

#include <functional>
#include <vector>

#include "http.h"

namespace servx {

class HttpRequest;

class HttpPhaseRunner {
public:
    using http_phase_handler_t = std::function<int(HttpRequest*)>;

    HttpPhaseRunner(const HttpPhaseRunner&) = delete;
    HttpPhaseRunner(HttpPhaseRunner&&) = delete;
    HttpPhaseRunner& operator=(const HttpPhaseRunner&) = delete;
    HttpPhaseRunner& operator=(HttpPhaseRunner&&) = delete;

    ~HttpPhaseRunner() = default;

    void register_handler(HttpPhase phase,
        const http_phase_handler_t& h);

    void init();

    void run(HttpRequest* req);

    static int find_config_handler(HttpRequest* req);

    static HttpPhaseRunner* instance() { return runner; }

private:
    HttpPhaseRunner() = default;

    int generic_phase_checker(HttpRequest* req);
//  int rewrite_phase_checker(HttpRequest* req);
//  int access_phase_checker(HttpRequest* req);
    int content_phase_checker(HttpRequest* req);

    std::vector<http_phase_handler_t> phase_handlers[HTTP_LOG_PHASE + 1];

    static HttpPhaseRunner* runner;
};

inline void HttpPhaseRunner::register_handler(
    HttpPhase phase, const http_phase_handler_t& h) {
    phase_handlers[phase].push_back(h);
}

}

#endif

#ifndef _HTTP_PHASE_H_
#define _HTTP_PHASE_H_

#include <functional>

#include "http_request.h"

namespace servx {

enum HttpPhase {
    HTTP_POST_READ_PHASE,
    HTTP_FIND_CONFIG_PHASE,
    HTTP_ACCESS_PHASE,
    HTTP_CONTENT_PHASE,
    HTTP_LOG_PHASE
};

using phase_handler_t = std::function<int (const HttpRequest&)>;

class HttpPhaseHandler {
};

void http_run_phases(HttpRequest* req);

}

#endif

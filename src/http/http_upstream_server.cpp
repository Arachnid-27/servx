#include "http_upstream_server.h"

namespace servx {

Buffer* HttpUpstreamServer::get_body_buf() {
    if (free_body_bufs.empty()) {
        // TODO: custom
        all_bufs.emplace_back(4096);
        free_body_bufs.emplace_back(&all_bufs.back());
    }

    Buffer *buf = free_body_bufs.back();
    free_body_bufs.pop_back();
    return buf;
}

}

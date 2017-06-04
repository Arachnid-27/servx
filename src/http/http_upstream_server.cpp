#include "http_upstream_server.h"

#include "logger.h"

namespace servx {

Buffer* HttpUpstreamServer::get_body_buf() {
    if (free_body_bufs.empty()) {
        // TODO: custom
        all_bufs.emplace_back(new Buffer(4096));
        free_body_bufs.emplace_back(all_bufs.back().get());
    }

    Buffer *buf = free_body_bufs.back();
    free_body_bufs.pop_back();

    return buf;
}

void HttpUpstreamServer::ret_body_buf(Buffer* buf) {
    buf->reset();
    free_body_bufs.push_back(buf);
}

}

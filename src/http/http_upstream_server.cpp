#include "http_upstream_server.h"

namespace servx {

Buffer* HttpUpstreamServer::get_buffer() {
    if (free_bufs.empty()) {
        // TODO: custom
        all_bufs.emplace_back(4096);
        return &all_bufs.back();
    }

    Buffer *buf = free_bufs.back();
    free_bufs.pop_back();

    return buf;
}

void HttpUpstreamServer::ret_buffer(Buffer* buf) {
    buf->reset();
    free_bufs.push_back(buf);
}

}

#include "http_writer.h"

#include "logger.h"

namespace servx {

int HttpWriter::write() {
    if (out.empty()) {
        return end ? SERVX_OK : SERVX_AGAIN;
    }

    auto iter = conn->send_chain(out.begin(), out.end());
    if (conn->is_error()) {
        Logger::instance()->info("connection error");
        return SERVX_ERROR;
    }
    out.erase(out.begin(), iter);

    return out.empty() && end ? SERVX_OK : SERVX_AGAIN;
}

}

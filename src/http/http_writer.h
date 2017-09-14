#ifndef _HTTP_WRITER_
#define _HTTP_WRITER_

#include <list>

#include "buffer.h"
#include "connection.h"

namespace servx {

class HttpWriter {
public:
    HttpWriter(Connection* c): conn(c), end(false) {}

    void push(Buffer* buf) { out.push_back(buf); }

    void push(const std::list<Buffer*>& bufs) {
        out.insert(out.end(), bufs.begin(), bufs.end());
    }

    int write();

    int set_end() {
        end = true;
        return out.empty() ? SERVX_OK : SERVX_AGAIN;
    }

private:
    std::list<Buffer*> out;
    Connection *conn;
    bool end;
};

}

#endif

#include "http_parse.h"

namespace servx {

int http_parse_request_line(HttpRequest* req) {
    Buffer *buf = req->get_recv_buf();
    char *last = buf->get_last();
    int state = req->get_parse_state();
    char ch;

    for (char *p = buf->get_pos(); p < last; ++p) {
        ch = *p;

        switch (state) {
        }
    }

    return PARSE_OK;
}

}

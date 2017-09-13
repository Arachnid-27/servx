#include "buffer.h"

#include <cstring>

namespace servx {

Buffer::Buffer(uint32_t cap) {
    start = new char[cap];
    end = start + cap;
    pos = last = start;
}

void Buffer::shrink() {
    if (start != pos) {
        uint32_t size = get_size();
        memmove(start, pos, size);
        pos = start;
        last = pos + size;
    }
}

void Buffer::enlarge(uint32_t cap) {
    if (get_capacity() < cap) {
        uint32_t size = get_size();
        char *old = start;
        start = new char[cap];
        memcpy(start, pos, size);
        pos = start;
        end = start + cap;
        last = pos + size;
        delete[] old;
    }
}

};

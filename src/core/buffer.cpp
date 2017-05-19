#include "buffer.h"

#include <cstring>

namespace servx {

Buffer::Buffer(uint32_t cap): deleteable(true) {
    start = new char[cap];
    end = start + cap;
    pos = last = start;
}

Buffer::Buffer(char* data, uint32_t size, bool del): deleteable(del) {
    if (del) {
        start = new char[size];
        memcpy(start, data, size);
        pos = start;
    } else {
        start = pos = data;
    }
    end = last = data + size;
}

void Buffer::shrink() {
    if (deleteable && start != pos) {
        uint32_t size = get_size();
        memmove(start, pos, size);
        pos = start;
        last = pos + size;
    }
}

void Buffer::enlarge(uint32_t cap) {
    if (deleteable && get_capacity() < cap) {
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

Buffer::~Buffer() {
    if (deleteable) {
        delete[] start;
    }
}

};

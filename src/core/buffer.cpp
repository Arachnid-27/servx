#include "buffer.h"

#include <cstring>

namespace servx {

Buffer::Buffer(int sz): size(sz) {
    start = new char[sz];
    end = start + size;
    reset();
}

void Buffer::reset() {
    pos = last = start;
}

void Buffer::shrink() {
    if (start != pos) {
        auto offset = last - pos;
        memmove(start, pos, offset);
        pos = start;
        last = pos + offset;
    }
}

void Buffer::resize(int sz) {
    if (sz > size) {
        auto offset = last - pos;
        char *new_start = new char[sz];
        memcpy(new_start, pos, offset);
        start = pos = new_start;
        end = new_start + sz;
        last = pos + offset;
        size = sz;
    }
}

Buffer::~Buffer() {
    delete[] start;
}

};

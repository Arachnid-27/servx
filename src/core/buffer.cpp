#include "buffer.h"

namespace servx {

Buffer::Buffer(int sz): size(sz) {
    start = new char[sz];
    end = start + size;
    reset();
}

void Buffer::reset() {
    pos = last = start;
}

Buffer::~Buffer() {
    delete[] start;
}

};

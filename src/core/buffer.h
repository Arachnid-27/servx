#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <cstdint>

namespace servx {

class Buffer {
public:
    explicit Buffer(uint32_t cap);
    Buffer(char* data, uint32_t size, bool del);

    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&&) = delete;

    ~Buffer();

    char* get_pos() const { return pos; }
    void set_pos(char* p) { pos = p; }

    char* get_last() const { return last; }
    void set_last(char* p) { last = p; }

    char *get_end() const { return end; }

    uint32_t get_size() const { return last - pos; }
    uint32_t get_capacity() const { return end - start; }
    uint32_t get_remain() const { return end - last; }

    void shrink();
    void enlarge(uint32_t cap);

private:
    char *start;
    char *end;
    char *pos;
    char *last;
    bool deleteable;
};

}

#endif

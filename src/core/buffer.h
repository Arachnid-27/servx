#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <cstdint>

namespace servx {

class Buffer {
public:
    explicit Buffer(uint32_t cap);
    Buffer(char* data, uint32_t size, bool del);

    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&) = default;
    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&&) = default;

    ~Buffer();

    char* get_pos() const { return pos; }
    void set_pos(char* p) { pos = p; }

    char* get_last() const { return last; }
    void set_last(char* p) { last = p; }

    char *get_start() const { return start; }
    char *get_end() const { return end; }

    void move_pos(int n) { pos += n; }
    void move_last(int n) { last += n; }

    uint32_t get_size() const { return last - pos; }
    uint32_t get_capacity() const { return end - start; }
    uint32_t get_remain() const { return end - last; }

    void reset() { pos = last = start; }
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

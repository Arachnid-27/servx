#ifndef _BUFFER_H_
#define _BUFFER_H_

namespace servx {

class Buffer {
public:
    Buffer(int sz);

    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&&) = delete;

    ~Buffer();

    char* get_pos() const { return pos; }
    void set_pos(char* p) { pos = p; }

    char* get_last() const { return last; }
    void set_last(char* p) { last = p; }

    int get_size() const { return size; }
    int get_remain() const { return end - last; }

    void reset();
    void shrink();
    void enlarge(int sz);

private:
    int size;
    char *start;
    char *end;
    char *pos;
    char *last;
};

}

#endif

#ifndef _FILE_H_
#define _FILE_H_

#include <fcntl.h>
#include <unistd.h>

#include <string>

namespace servx {

class File {
public:
    File(int f): fd(f) {}

    File(const std::string& s): pathname(s) {}
    File(const char* s): pathname(s) {}

    File(const File&) = delete;
    File(File&&) = default;
    File& operator=(const File&) = delete;
    File& operator=(File&&) = default;

    ~File();

    const std::string& get_pathname() const { return pathname; }

    bool open(int flags);

    bool open(int flags, mode_t mode);

    int read(char* buf, int count) { return ::read(fd, buf, count); }

    int write(char* buf, int count) { return ::write(fd, buf, count); }

private:
    int fd;
    std::string pathname;
};

inline bool File::open(int flags) {
    fd = ::open(pathname.c_str(), flags);
    return fd == -1;
}

inline bool File::open(int flags, mode_t mode) {
    fd = ::open(pathname.c_str(), flags, mode);
    return fd == -1;
}

bool operator==(const File& lhs, const File& rhs);

bool operator!=(const File& lhs, const File& rhs);

}

#endif

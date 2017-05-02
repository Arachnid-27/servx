#ifndef _FILE_H_
#define _FILE_H_

#include <fcntl.h>
#include <unistd.h>
#include <string>

#define OPEN_MODE_RDONLY O_RDONLY
#define OPEN_MODE_WDONLY O_WDONLY
#define OPEN_MODE_RDWR   O_RDWR

class File {
public:
    File(const std::string& s): pathname(s) {}

    File(const char* s): pathname(s) {}

    ~File() {
        if (fd != -1) {
            ::close(fd);
        }
    }

    const std::string& get_pathname() const { return pathname; }

    bool open(int mode) {
        fd = ::open(pathname.c_str(), mode);
        return fd == -1;
    }

    char read() {
        char ch;
        int c = ::read(fd, &ch, 1);
        return c == 1 ? ch : EOF;
    }

    int read(char* buf, int count) {
        return ::read(fd, buf, count);
    }
private:
    int fd;
    std::string pathname;
};

bool operator==(const File& lhs, const File& rhs);

bool operator!=(const File& lhs, const File& rhs);

#endif

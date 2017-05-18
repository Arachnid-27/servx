#ifndef _FILE_H_
#define _FILE_H_

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <memory>
#include <string>

namespace servx {

class File {
public:
    File(int f): fd(f) {}

    File(const std::string& s): pathname(s) {}
    File(std::string&& s): pathname(std::move(s)) {}
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

    bool file_status();

    bool is_dir() const { return info != nullptr && (S_ISDIR(info->st_mode)); }

    bool is_file() const { return info != nullptr && (S_ISREG(info->st_mode)); }

    long get_file_size() const;

    long get_modify_time() const;

private:
    int fd;
    std::string pathname;
    std::unique_ptr<struct stat> info;
};

inline bool File::open(int flags) {
    fd = ::open(pathname.c_str(), flags);
    return fd == -1;
}

inline bool File::open(int flags, mode_t mode) {
    fd = ::open(pathname.c_str(), flags, mode);
    return fd == -1;
}

inline long File::get_file_size() const {
    return info == nullptr ? 0 : info->st_size;
}

inline long File::get_modify_time() const {
    if (info == nullptr) {
        return 0;
    }
    return info->st_mtim.tv_sec * 1000 + info->st_mtim.tv_nsec / 1000;
}

inline bool operator==(const File& lhs, const File& rhs) {
    return lhs.get_pathname() == rhs.get_pathname();
}

inline bool operator!=(const File& lhs, const File& rhs) {
    return !(lhs == rhs);
}

}

#endif

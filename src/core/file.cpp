#include "file.h"

namespace servx {

File::~File() {
    if (fd != -1) {
        ::close(fd);
    }
}

inline bool operator==(const File& lhs, const File& rhs) {
    return lhs.get_pathname() == rhs.get_pathname();
}

inline bool operator!=(const File& lhs, const File& rhs) {
    return !(lhs == rhs);
}

}

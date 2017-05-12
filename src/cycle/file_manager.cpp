#include "file_manager.h"

#include <algorithm>

#include "logger.h"

namespace servx {

FileManager* FileManager::manager = new FileManager;

void FileManager::push_file(const std::string& s) {
    auto beg = open_files.begin();
    auto end = open_files.end();
    auto comp = [s](const File& f) { return f.get_pathname() == s; };

    if (std::find_if(beg, end, comp) == end) {
        open_files.emplace_back(s);
    } else {
        Logger::instance()->info("%s exists, ignore", s.c_str());
    }
}

}

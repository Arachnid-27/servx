#ifndef _FILE_MANAGER_
#define _FILE_MANAGER_

#include <vector>

#include "file.h"

namespace servx {

class FileManager {
public:
    FileManager(const FileManager&) = delete;
    FileManager(FileManager&&) = delete;
    FileManager& operator=(const FileManager&) = delete;
    FileManager& operator=(FileManager&&) = delete;

    ~FileManager() = default;

    void push_file(const std::string& s);

    static FileManager* instance() { return manager; }

private:
    FileManager() = default;

private:
    std::vector<File> open_files;

    static FileManager *manager;
};

}

#endif

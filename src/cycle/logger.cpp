#include "logger.h"

#include <cstdio>

#include "clock.h"

namespace servx {

Logger* Logger::logger = new Logger;

void Logger::log(const char* level, const char* fmt, va_list args) {
    const char* time = Clock::instance()->get_current_log_time().c_str();

    int sz = snprintf(buf, 1024, "[%s] #%d# %s : ", level, pid, time);

    sz += vsnprintf(buf + sz, 1024 - sz, fmt, args);

    if (sz == 1024) {
        buf[1023] = '\n';
    } else {
        buf[sz] = '\n';
        buf[++sz] = '\0';
    }

    if (!open) {
        std_err.write(buf, sz);
        return;
    }

    for (auto &f : files) {
        f.write(buf, sz);
    }
}

void Logger::open_files() {
    if (files.empty()) {
        files.emplace_back("log/error.log");
    }

    auto iter = files.begin();
    int flags = O_RDWR | O_CREAT | O_APPEND;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

    while (iter != files.end()) {
        if (iter->open(flags, mode) != 0) {
            warn("open %s failed, ignore", iter->get_pathname().c_str());
            iter = files.erase(iter);
        } else {
            ++iter;
        }
    }

    if (!files.empty()) {
        open = true;
    }
    files.shrink_to_fit();
}

}

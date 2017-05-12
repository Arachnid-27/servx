#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <unistd.h>

#include <cstdarg>
#include <vector>

#include "file.h"

#define log_with_level(level)           \
    va_list args;                       \
    va_start(args, fmt);                \
    log(level, fmt, args);              \
    va_end(args)

namespace servx {

enum LogLevel {
    LOG_ALERT,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
};

class Logger {
public:
    Logger(): pid(getpid()), std_err(STDERR_FILENO), open(false) {}

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

    ~Logger() = default;

    void debug(const char* fmt, ...) { log_with_level("debug"); }
    void info(const char* fmt, ...) { log_with_level("info"); }
    void warn(const char* fmt, ...) { log_with_level("warn"); }
    void error(const char* fmt, ...) { log_with_level("error"); }
    void alert(const char* fmt, ...) { log_with_level("alert"); }

    void push_file(const std::string& s) { files.emplace_back(s); }

    void open_files();

    static Logger* instance() { return logger; }

private:
    void log(const char* level, const char* fmt, va_list args);

private:
    char buf[1024];
    pid_t pid;
    File std_err;
    std::vector<File> files;
    bool open;

    static Logger* logger;
};

}

#endif

#include "cycle.h"

Cycle* Cycle::cycle = new Cycle;

bool Cycle::reload() {
    Clock::instance()->update();

    ConfParser::instance()->open("servx.conf");

    if (!ConfParser::instance()->parse()) {
        // error_log
        return false;
    }

    for (auto& f : open_files) {
        if (!f.open(OPEN_MODE_RDWR)) {
            return false;
        }
    }

    for (auto& s : listenings) {
    }

    return true;
}

void Cycle::open_file(const std::string& s) {
    auto beg = open_files.begin();
    auto end = open_files.end();
    auto comp = [s](const File& f) { return f.get_pathname() == s; };

    if (std::find_if(beg, end, comp) == end) {
        open_files.emplace_back(s);
    } else {
        // error_log
    }
}

Process& Cycle::spawn_process(const process_task_t& p) {
    processes.emplace_back(p);
    return processes.back();
}

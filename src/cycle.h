#ifndef _CYCLE_H_
#define _CYCLE_H_

#include <algorithm>
#include <memory>
#include <queue>
#include <vector>

#include "clock.h"
#include "file.h"
#include "process.h"

namespace servx {

class Cycle {
public:
    Cycle(const Cycle&) = delete;

    bool reload();

    void open_file(const std::string& s);

    Process& spawn_process(const process_task_t& p);

public:
    static Cycle* instance() { return cycle; }

private:
    Cycle() = default;

private:
    std::vector<Process> processes;
    std::vector<File> open_files;

private:
    static Cycle* cycle;
};

}

#endif

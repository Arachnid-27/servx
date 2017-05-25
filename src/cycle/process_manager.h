#ifndef _PROCESS_MANAGER_H_
#define _PROCESS_MANAGER_H_

#include <vector>

#include "process.h"

namespace servx {

class ProcessManager {
public:
    ProcessManager(const ProcessManager&) = delete;
    ProcessManager(ProcessManager&&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;
    ProcessManager& operator=(ProcessManager&&) = delete;

    ~ProcessManager() = default;

    void push(const process_task_t& t) { processes.emplace_back(t); }

    bool spawn();

    static ProcessManager* instance() { return manager; }

private:
    ProcessManager() = default;

    std::vector<Process> processes;

    static ProcessManager* manager;
};

}

#endif

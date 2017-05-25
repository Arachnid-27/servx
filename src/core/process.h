#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <functional>

#include <sched.h>

namespace servx {

using process_task_t = std::function<void()>;

class Process {
public:
    Process(const process_task_t& p): proc(p) {}

    Process(const Process&) = delete;
    Process(Process&&) = default;
    Process& operator=(const Process&) = delete;
    Process& operator=(Process&&) = default;

    ~Process() = default;

    int get_status() const { return status; }
    void set_statue(int s) { status = s; }

    bool run(cpu_set_t* set);

private:
    pid_t pid;
    int status;
    process_task_t proc;
};

}

#endif

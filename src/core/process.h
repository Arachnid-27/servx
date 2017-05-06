#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <functional>

namespace servx {

using process_task_t = std::function<void ()>;

class Process {
public:
    Process(const process_task_t& p): proc(p) {}

    int get_status() const { return status; }

    void set_statue(int s) { status = s; }

    bool run();

private:
    pid_t pid;
    int status; 
    process_task_t proc;
};

}

#endif

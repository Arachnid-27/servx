#ifndef _CYCLE_H_
#define _CYCLE_H_

#include "core.h"

class Cycle {
    using conn_ptr = std::shared_ptr<Connection>;

public:
    void open_file(const std::string& s);

    Process& spawn_process(const process_task_t& p);

public:
    static Cycle* instance() { return cycle; }

private:
    Cycle() = default;
    
    Cycle(const Cycle&) = delete;

private:
    std::vector<Process> processes;
    std::vector<File> open_files;
    std::vector<Listening> listenings;
    std::queue<conn_ptr> free_connections;
    std::queue<conn_ptr> reusable_connections;

private:
    static Cycle* cycle;
};

#endif

#include "core.h"
#include "conf_parser.h"

int main(int argc, char* argv[]) {
    Clock::instance()->update();

    ConfParser::instance()->open("servx.conf");

    if (!ConfParser::instance()->parse()) {
        // error_log
        exit(EXIT_FAILURE);
    }

    auto conf = ModuleManager::instance()->get_conf<MainCoreModule>();

    if (conf->daemon) {
        daemonize();
    }

    auto cycle = Cycle::instance();

    memcpy(argv[0], "servx-worker", sizeof("servx-worker"));

    for (auto i = 0; i < conf->worker; ++i) {
        auto process = cycle->spawn_process(worker_process_cycle);
        if (!process.run()) {
            exit(EXIT_FAILURE);
        }
    }

    memcpy(argv[0], "servx-master", sizeof("servx-master"));

    master_process_cycle();

    return 0;
}

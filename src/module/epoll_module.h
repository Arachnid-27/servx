#ifndef _EPOLL_MODULE_H_
#define _EPOLL_MODULE_H_

#include <sys/epoll.h>

#include "event_module.h"

namespace servx {

struct EpollModuleConf {
    int epoll_events;
};

class EpollModule: public ModuleWithConf<EventModule,
                          EpollModuleConf,
                          EPOLL_MODULE> {
public:
    EpollModule(): ModuleWithConf(
        {
            new Command(EVENT_BLOCK,
                        "epoll_events",
                        lambda_handler(epoll_events_handler), 1)
        }) {}

    bool init_conf() override;

    bool init_process() override;

    bool add_event(Event* ev, int flags) override;

    bool del_event(Event* ev, int flags) override;

    bool add_connection(Connection* c) override;

    bool del_connection(Connection* c) override;

    bool process_events() override;

    int epoll_events_handler(command_vals_t);

private:
    int ep;
    epoll_event *event_list;
};

}

#endif

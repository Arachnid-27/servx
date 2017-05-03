#ifndef _EPOLL_MODULE_H_
#define _EPOLL_MODULE_H_


#include "clock.h"
#include "event_module.h"

#include <errno.h>
#include <sys/epoll.h>

#include <unordered_map>


struct EpollModuleConf {
    int epoll_events;
};


class EpollModule: public ModuleWithConf<EventModule,
                          EpollModuleConf,
                          ModuleIndex::EPOLL_MODULE> {
public:
    EpollModule(): ModuleWithConf(
        {
            new Command(ModuleType::EVENT_MODULE, "epoll_events",
                        epoll_events_handler, 1)
        },
        new EpollModuleConf) {}

    virtual bool init_conf();

    virtual bool init_process();

    virtual bool add_event(Event* ev, int flags);

    virtual bool del_event(Event* ev, int flags);

    virtual bool add_connection(Connection* c);

    virtual bool del_connection(Connection* c);

    virtual bool process_events();

public:
    static bool epoll_events_handler(command_vals_t);

private:
    int ep;
    epoll_event *event_list;
};


#endif

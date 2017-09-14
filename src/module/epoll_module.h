#ifndef _EPOLL_MODULE_H_
#define _EPOLL_MODULE_H_

#include <sys/epoll.h>

#include "event_module.h"

namespace servx {

struct EpollModuleConf {
    int epoll_events;
};

class EpollModule: public EventModule {
public:
    bool init_conf() override;

    bool init_process() override;

    bool add_event(Event* ev) override;

    bool del_event(Event* ev) override;

    bool add_connection(Connection* c) override;

    bool del_connection(Connection* c) override;

    bool process_events() override;

    static EpollModuleConf conf;
    static EpollModule instance;
    static std::vector<Command*> commands;

private:
    int ep;
    epoll_event *event_list;
};

namespace command {

class UseEpoll: public Command {
public:
    UseEpoll(): Command("event", "use_epoll", 0) {}

    bool execute(const command_args_t& v) override {
        EventCoreModule::conf.module = &EpollModule::instance;
        return true;
    }
};

class EpollEvents: public Command {
public:
    EpollEvents(): Command("event", "epoll_events", -1) {}

    bool execute(const command_args_t& v) override {
        return integer_parse(&EpollModule::conf,
            &EpollModuleConf::epoll_events, v);
    }
};

}

}

#endif

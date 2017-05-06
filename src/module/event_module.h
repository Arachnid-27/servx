#ifndef _EVENT_MODULE_
#define _EVENT_MODULE_

#include "connection.h"
#include "core_module.h"
#include "event.h"

namespace servx {

class EventModule: public Module {
public:
    virtual bool add_event(Event* ev, int flags) = 0;

    virtual bool del_event(Event* ev, int flags) = 0;

    virtual bool add_connection(Connection* c) = 0;

    virtual bool del_connection(Connection* c) = 0;

    virtual bool process_events() = 0;

protected:
    EventModule(const std::initializer_list<Command*>& v)
        : Module(EVENT_MODULE, v) {}
};

struct MainEventConf {
    int time_resolution;
    int connections;
};

class MainEventModule: public ModuleWithConf<CoreModule,
                                             MainEventConf,
                                             MAIN_EVENT_MODULE> {
public:
    MainEventModule(): ModuleWithConf(
        {
            new Command(CORE_BLOCK,
                        "event",
                        lambda_handler(event_handler), 1),
            new Command(EVENT_BLOCK,
                        "timer_resolution",
                        lambda_handler(timer_resolution_handler), 1),
            new Command(EVENT_BLOCK,
                        "connections",
                        lambda_handler(connections_handler), 1),
        }) {}

    bool init_conf() override;

    bool init_module() override;

    bool init_process() override;

    int event_handler(command_vals_t v);

    int timer_resolution_handler(command_vals_t v);

    int connections_handler(command_vals_t v);

    static void sig_timer_handler(int sig);
};

}

#endif

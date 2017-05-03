#ifndef _EVENT_MODULE_
#define _EVENT_MODULE_


#include "module.h"
#include "core_module.h"
#include "connection.h"
#include "event.h"


class EventModule: public Module {
public:
    virtual bool add_event(Event* ev, int flags) = 0;

    virtual bool del_event(Event* ev, int flags) = 0;

    virtual bool add_connection(Connection* c) = 0;

    virtual bool del_connection(Connection* c) = 0;

    virtual bool process_events() = 0;

protected:
    EventModule(const std::initializer_list<Command*>& v): Module(EVENT_MODULE, v) {}
};


struct EventModuleConf {
    int time_resolution;
    unsigned int connections;
    std::string use;
};


class MainEventModule: public ModuleWithConf<CoreModule,
                                             EventModuleConf,
                                             ModuleIndex::MAIN_EVENT_MODULE> {
public:
    MainEventModule(): ModuleWithConf(
        {
            new Command(ModuleType::CORE_MODULE, "event",
                        event_handler, 1),
            new Command(ModuleType::EVENT_MODULE, "timer_resolution",
                        timer_resolution_handler, 1),
            new Command(ModuleType::EVENT_MODULE, "connections",
                        connections_handler, 1),
            new Command(ModuleType::EVENT_MODULE, "use",
                        use_handler, 1)
        },
        nullptr) {}

public:
    static bool event_handler(command_vals_t v);

    static bool timer_resolution_handler(command_vals_t v);

    static bool connections_handler(command_vals_t);

    static bool use_handler(command_vals_t);
};


#endif

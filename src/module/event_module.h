#ifndef _EVENT_MODULE_
#define _EVENT_MODULE_

#include "connection.h"
#include "module.h"
#include "module_manager.h"

namespace servx {

struct EventCoreConf {
    int time_resolution;
    int connections;
    bool multi_accept;
    EventModule *module;
};

class EventCoreModule: public CoreModule {
public:
    bool init_conf() override;

    bool init_module() override;

    bool init_process() override;

    static EventCoreConf conf;
    static EventCoreModule instance;
    static std::vector<Command*> commands;
};

namespace command {

class Event: public Command {
public:
    Event(): Command("core", "event", 0) {}

    bool execute(const command_args_t& v) override { return true; }
};

class TimerResolution: public Command {
public:
    TimerResolution(): Command("event", "timer_resolution", 1) {}

    bool execute(const command_args_t& v) override {
        return integer_parse(&EventCoreModule::conf,
            &EventCoreConf::time_resolution, v);
    }
};

class Connections: public Command {
public:
    Connections(): Command("event", "connections", 1) {}

    bool execute(const command_args_t& v) override {
        return integer_parse(&EventCoreModule::conf,
            &EventCoreConf::connections, v);
    }
};

class MultiAccept: public Command {
public:
    MultiAccept(): Command("event", "multi_accept", 1) {}

    bool execute(const command_args_t& v) override {
        return boolean_parse(&EventCoreModule::conf,
            &EventCoreConf::multi_accept, v);
    }
};

}

inline bool add_event(Event* ev) {
    if (ev->is_active()) {
        return true;
    }
    return EventCoreModule::conf.module->add_event(ev);
}

inline bool del_event(Event* ev) {
    if (!ev->is_active()) {
        return true;
    }
    return EventCoreModule::conf.module->del_event(ev);
}

inline bool add_connection(Connection* c) {
    return EventCoreModule::conf.module->add_connection(c);
}

inline bool del_connection(Connection* c) {
    return EventCoreModule::conf.module->del_connection(c);
}

inline void process_event() {
    EventCoreModule::conf.module->process_events();
}

}

#endif

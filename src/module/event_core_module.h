#ifndef _EVENT_CORE_MODULE_H_
#define _EVENT_CORE_MODULE_H_

#include "core.h"

struct EventCoreModuleConf {
    int time_resolution;
};

class EventCoreModule:
    public CoreModule<EventCoreModuleConf, ModuleIndex::EVENT_CORE_MODULE> {
public:
    EventCoreModule():
        CoreModule({
                    new Command("event", event_handler),
                    new Command("timer_resolution", timer_resolution_handler)
                   },
                   nullptr) {}

public:
    static bool event_handler(const std::vector<std::string>& v);

    static bool timer_resolution_handler(const std::vector<std::string>& v);
};

#endif

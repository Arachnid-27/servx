#ifndef _MODULE_H_
#define _MODULE_H_

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <initializer_list>

enum ModuleIndex {
    MAIN_CORE_MODULE,
    EVENT_CORE_MODULE,
    MAIN_EVENT_MODULE,
    EPOLL_MODULE,
    NULL_MODULE
};

using command_handler_t = std::function<bool (const std::vector<std::string>&)>;
using command_post_handler_t = std::function<bool ()>;

class Command {
public:
    Command(const std::string& s, const command_handler_t& ch, int n = -1,
            const command_post_handler_t& cph = __empty_post_handler):
        name(s), handler(ch), args(n), post_handler(cph) {}

    const std::string& get_name() const { return name; }

    int args_count() const { return args; }

    bool execute(const std::vector<std::string>& v) const { return handler(v); }

    bool post_execute() const { return post_handler(); }

public:
    static bool __empty_post_handler() { return true; }

private:
    std::string name;
    command_handler_t handler;
    int args;
    command_post_handler_t post_handler;
};

class Module {
protected:
    Module(const std::initializer_list<Command*>& v): commands(v) {}

public:
    virtual bool init_module() { return true; }

    virtual bool init_process() { return true; }

    virtual bool exit_process() { return true; }

    virtual bool exit_module() { return true; }

    const std::vector<Command*>& get_commands() const { return commands; }

protected:
    std::vector<Command*> commands;
};

template <class Conf, ModuleIndex Index>
class ModuleWithConf: public Module {
protected:
    ModuleWithConf(const std::initializer_list<Command*>& v, Conf* c): Module(v), conf(c) {}

public:
    Conf* get_conf() const { return conf; }

    virtual bool init_conf() { return true; };

public:
    constexpr static ModuleIndex get_index() { return Index; }

protected:
    Conf *conf;
};

template <class Conf, ModuleIndex index>
class CoreModule: public ModuleWithConf<Conf, index> {
protected:
    CoreModule(const std::initializer_list<Command*>& v, Conf* c): ModuleWithConf<Conf, index>(v, c) {}
};

template <class Conf, ModuleIndex index>
class EventModule: public ModuleWithConf<Conf, index> {
protected:
    EventModule(const std::initializer_list<Command*>& v, Conf* c): ModuleWithConf<Conf, index>(v, c) {}
};

class ModuleManager {
public:
    Command* find_command(const std::string& name) const;
    
    template <class T, class Conf = decltype(((T*) nullptr)->get_conf())>
    Conf get_conf() const;

    bool for_each(std::function<bool (Module*)> func);

public:
    static ModuleManager* instance() { return manager; }

private:
    ModuleManager();

    void create_module(ModuleIndex index, Module* module);

private:
    Module* modules[ModuleIndex::NULL_MODULE];
    std::unordered_map<std::string, Command*> commands;

private:
    static ModuleManager* manager;
};

template <class T, class Conf>
inline Conf ModuleManager::get_conf() const {
    return static_cast<T*>(modules[T::get_index()])->get_conf();
}

#include "main_core_module.h"
#include "event_core_module.h"
#include "main_event_module.h"
#include "epoll_module.h"

#endif

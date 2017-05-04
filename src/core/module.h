#ifndef _MODULE_H_
#define _MODULE_H_

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <initializer_list>

namespace servx {

enum ModuleIndex {
    MAIN_CORE_MODULE,
    MAIN_EVENT_MODULE,
    EPOLL_MODULE,
    NULL_MODULE
};

enum ModuleType {
    CORE_MODULE,
    EVENT_MODULE,
    HTTP_MODULE
};

using command_vals_t = const std::vector<std::string>&;
using command_handler_t = std::function<bool (command_vals_t)>;
using command_post_handler_t = std::function<bool ()>;

class Command {
public:
    Command(const ModuleType t, const std::string& s,
            const command_handler_t& ch, const int n = -1,
            const command_post_handler_t& cph = __empty_post_handler)
        : type(t), name(s),
          handler(ch), args(n),
          post_handler(cph) {}

    const std::string& get_name() const { return name; }

    int args_count() const { return args; }

    bool execute(command_vals_t v) const { return handler(v); }

    bool post_execute() const { return post_handler(); }

public:
    static bool __empty_post_handler() { return true; }

private:
    ModuleType type;
    std::string name;
    command_handler_t handler;
    int args;
    command_post_handler_t post_handler;
};

class Module {
public:
    virtual bool init_conf() { return true; }

    virtual bool init_module() { return true; }

    virtual bool init_process() { return true; }

    virtual bool exit_process() { return true; }

    virtual bool exit_module() { return true; }

    ModuleType get_type() const { return type; }

    const std::vector<Command*>& get_commands() const { return commands; }

protected:
    Module(ModuleType mt, const std::initializer_list<Command*>& v)
        : type(mt), commands(v) {}

protected:
    ModuleType type;
    std::vector<Command*> commands;
};

template <class MT, class Conf, ModuleIndex MI>
class ModuleWithConf: public MT {
public:
    Conf* get_conf() { return &conf; }

public:
    constexpr static ModuleIndex get_index() { return MI; }

protected:
    ModuleWithConf(const std::initializer_list<Command*>& v): MT(v) {}

protected:
    Conf conf;
};

}

#endif

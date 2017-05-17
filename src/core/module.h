#ifndef _MODULE_H_
#define _MODULE_H_

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <initializer_list>

#define ERROR_COMMAND -1

#define lambda_handler(handler)                                 \
        [this](command_vals_t v) { return this->handler(v); }

#define lambda_post_handler(handler)                            \
        [this]() { return this-handler(); }

namespace servx {

enum ModuleIndex {
    MAIN_CORE_MODULE,
    MAIN_EVENT_MODULE,
    EPOLL_MODULE,
    MAIN_HTTP_MODULE,
    HTTP_STATIC_MODULE,
    NULL_MODULE
};

enum ModuleType {
    CORE_MODULE,
    EVENT_MODULE,
    HTTP_MODULE
};

enum CommandBlock {
    NULL_BLOCK,
    CORE_BLOCK,
    EVENT_BLOCK,
    HTTP_BLOCK,
    SERVER_BLOCK,
    LOCATION_BLOCK,
    ADDRESS_BLOCK
};

using command_vals_t = const std::vector<std::string>&;
using command_handler_t = std::function<int (command_vals_t)>;
using command_post_handler_t = std::function<bool ()>;

class Command {
public:
    Command(CommandBlock b, const char* s,
            const command_handler_t& ch, int n = -1,
            const command_post_handler_t& cph = __empty_post_handler)
        : block(b), name(s),
          handler(ch),  args(n),
          post_handler(cph) {}

    const std::string& get_name() const { return name; }

    int args_count() const { return args; }

    int execute(command_vals_t v) const { return handler(v); }

    bool post_execute() const { return post_handler(); }

    CommandBlock get_block_context() const { return block; }

public:
    static bool __empty_post_handler() { return true; }

private:
    CommandBlock block;
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

    // Todo virtual ~Module

    ModuleType get_type() const { return type; }

    const std::vector<Command*>& get_commands() const { return commands; }

protected:
    Module(ModuleType mt, const std::initializer_list<Command*>& v)
        : type(mt), commands(v) {}

protected:
    ModuleType type;
    // Todo raw pointer
    std::vector<Command*> commands;
};

class ModuleConf {};

template <typename ModuleType, typename Conf, int Index>
class ModuleWithConf: public ModuleType {
public:
    using conf_t = Conf;

    enum { index = Index };

public:
    Conf* get_conf() { return &conf; }

protected:
    ModuleWithConf(const std::initializer_list<Command*>& v): ModuleType(v) {}

protected:
    Conf conf;
};

}

#endif

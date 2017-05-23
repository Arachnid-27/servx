#ifndef _MODULE_H_
#define _MODULE_H_

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core.h"

#define lambda_handler(handler)                                 \
        [this](command_vals_t v) { return this->handler(v); }

#define lambda_post_handler(handler)                            \
        [this]() { return this->handler(); }

namespace servx {

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
using command_handler_t = std::function<int(command_vals_t)>;
using command_post_handler_t = std::function<bool()>;

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

    virtual ~Module() = default;

    ModuleType get_type() const { return type; }

    std::vector<Command*> get_commands() const;

protected:
    Module(ModuleType mt, const std::initializer_list<Command*>& v): type(mt) {
        std::for_each(v.begin(), v.end(),
            [this](Command* c) { commands.emplace_back(c); });
        commands.shrink_to_fit();
    }

protected:
    ModuleType type;
    std::vector<std::unique_ptr<Command>> commands;
};

inline std::vector<Command*> Module::get_commands() const {
    std::vector<Command*> vec;
    std::for_each(commands.begin(), commands.end(),
        [&](const std::unique_ptr<Command>& c) { vec.push_back(c.get()); });
    return vec;
}

class ModuleConf {};

template <typename T, typename U>
struct __conf_creator {
    U* create() { return new T(); }
};

template <typename U>
struct __conf_creator<void, U> {
    U* create() { return nullptr; }
};

template <typename ModuleType, typename Conf, int Index>
class ModuleWithConf: public ModuleType {
public:
    using conf_t = Conf;

    enum { index = Index };

public:
    Conf* get_conf() { return conf; }

protected:
    ModuleWithConf(const std::initializer_list<Command*>& v)
        : ModuleType(v), conf(__conf_creator<Conf, Conf>().create()) {}

protected:
    Conf *conf;
};

template <typename T, int T::* M, int R = NULL_BLOCK>
int set_conf_int(T* conf, const std::string& str) {
    int val = atoi(str.c_str());
    if (val <= 0) {
        return SERVX_ERROR;
    }
    conf->*M = val;
    return R;
}

template <typename T, void (T::* M)(int), int R = NULL_BLOCK>
int set_conf_int(T* conf, const std::string& str) {
    int val = atoi(str.c_str());
    if (val <= 0) {
        return SERVX_ERROR;
    }
    (conf->*M)(val);
    return R;
}

}

#endif

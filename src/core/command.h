#ifndef _COMMAND_H_
#define _COMMAND_H_

#include <string>
#include <vector>

namespace servx {

using command_args_t = std::vector<std::string>;

class Command {
public:
    const std::string& get_parent_name() const { return parent_name; }

    const std::string& get_name() const { return name; }

    int get_args() const { return args; }

    virtual bool execute(const command_args_t& v) = 0;

    virtual bool post_execute() { return true; }

protected:
    Command(const std::string& pn, const std::string& na, int ar)
        : parent_name(pn), name(na), args(ar) {}

private:
    std::string parent_name;
    std::string name;
    int args;
};

template <typename T>
bool integer_parse(T* conf, int T:: *field, const command_args_t& v) {
    int val = atoi(v[0].c_str());
    if (val <= 0) {
        return false;
    }
    conf->*field = val;
    return true;
}

template <typename T>
bool integer_parse(T* conf, void (T:: *func)(int), const command_args_t& v) {
    int val = atoi(v[0].c_str());
    if (val <= 0) {
        return false;
    }
    (conf->*func)(val);
    return true;
}

template <typename T>
bool boolean_parse(T* conf, bool T:: *field, const command_args_t& v) {
    if (v[0] == "on") {
        conf->*field = true;
    } else if (v[0] == "off") {
        conf->*field = false;
    } else {
        return false;
    }
    return true;
}

template <typename T>
bool boolean_parse(T* conf, void (T:: *func)(bool), const command_args_t& v) {
    if (v[0] == "on") {
        (conf->*func)(true);
    } else if (v[0] == "off") {
        (conf->*func)(false);
    } else {
        return false;
    }
    return true;
}

}

#endif

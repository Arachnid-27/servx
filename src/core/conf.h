#ifndef _CONF_H_
#define _CONF_H_

#include <string>
#include <vector>

namespace servx {

class ConfItem {
public:
    explicit ConfItem(const char* s): name(s) {}

    void push_value(const char* s) { values.emplace_back(s); }

    ConfItem* gen_sub_item(const char* s);

    const std::string& get_name() const { return name; }

    const std::vector<std::string>& get_values() const { return values; }

    const std::vector<ConfItem>& get_items() const { return items; }

private:
    std::string name;
    std::vector<std::string> values;
    std::vector<ConfItem> items;
};

}

#endif

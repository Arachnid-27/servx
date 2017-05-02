#ifndef _CONF_PARSER_H_
#define _CONF_PARSER_H_

#include "core.h"

class ConfItem {
public:
    ConfItem(const char* s): name(s) {}

    void push_value(const char* s) { values.push_back(s); }

    void push_sub_item(ConfItem&& item) { items.push_back(std::move(item)); }

    const std::string& get_name() const { return name; }

    const std::vector<std::string>& get_values() const { return values; }

    const std::vector<ConfItem>& get_items() const { return items; }

private:
    std::string name;
    std::vector<std::string> values;
    std::vector<ConfItem> items;
};

class ConfParser {
public:
    static ConfParser* instance() { return parser; }

public:
    ~ConfParser() {
        if (root) {
            delete root;
        }

        delete[] buf;
    }

    bool open(const char *pathname);

    bool parse();

private:
    enum {
        STATE_OK = 0,
        STATE_ERROR = 1,
        STATE_END = 2,
        STATE_FINISTH = 3,
        STATE_BLOCK_START = 4,
        STATE_BLOCK_END = 5
    };

private:
    ConfParser(): buf(new char[1024]) {};

    ConfParser(const ConfParser&) = delete;

    bool parse(ConfItem *parent);

    bool execute(const ConfItem& item, const ModuleManager& manager);

    int next_token();

private:
    static void __debug_print_conf(const ConfItem& item);

private:
    File *conf_file;
    ConfItem *root;
    char *buf;
    int len;

private:
    static ConfParser* parser;
};

#endif

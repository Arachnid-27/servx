#ifndef _CONF_PARSER_H_
#define _CONF_PARSER_H_

#include <memory>

#include "conf.h"
#include "file.h"

namespace servx {

class ConfParser {
private:
    enum {
        STATE_OK = 0,
        STATE_ERROR = 1,
        STATE_END = 2,
        STATE_FINISTH = 3,
        STATE_BLOCK_START = 4,
        STATE_BLOCK_END = 5
    };

public:
    ConfParser(const ConfParser&) = delete;
    ConfParser(ConfParser&&) = delete;
    ConfParser& operator=(const ConfParser&) = delete;
    ConfParser& operator=(ConfParser&&) = delete;

    ~ConfParser() = default;

    bool open(const char *pathname);

    bool parse();

    static ConfParser* instance() { return parser; }

private:
    ConfParser() = default;

    bool parse_item(ConfItem* parent);

    bool parse_value(ConfItem* item);

    bool process(const ConfItem& item, const std::string& block);

    int next_token();

private:
    std::unique_ptr<File> conf_file;
    std::unique_ptr<ConfItem> root;
    char buf[1024];

    static ConfParser* parser;
};

}

#endif

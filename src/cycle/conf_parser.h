#ifndef _CONF_PARSER_H_
#define _CONF_PARSER_H_

#include "conf.h"
#include "file.h"

namespace servx {

class ConfParser {
public:
    ConfParser(const ConfParser&) = delete;

    ~ConfParser();

    bool open(const char *pathname);

    bool parse();

    static ConfParser* instance() { return parser; }

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

    bool parse_item(ConfItem* parent);

    bool parse_value(ConfItem* item);

    bool process(const ConfItem& item, int block);

    int next_token();

private:
    File *conf_file;
    ConfItem *root;
    char *buf;
    int len;

    static ConfParser* parser;
};

}

#endif

#include "conf_parser.h"

#include "logger.h"
#include "module_manager.h"

namespace servx {

ConfParser* ConfParser::parser = new ConfParser;

ConfParser::~ConfParser() {
    delete root;
    delete[] buf;
}

bool ConfParser::parse() {
    if (conf_file == nullptr) {
        return false;
    }

    root = new ConfItem("");
    if (!parse_item(root)) {
        return false;
    }

    for (auto &c : root->get_items()) {
        if (!process(c, CORE_BLOCK)) {
            return false;
        }
    }

    return true;
}

bool ConfParser::process(const ConfItem& item, int block) {
    auto cmd = ModuleManager::instance()->find_command(item.get_name());

    if (cmd == nullptr) {
        return false;
    }

    if (cmd->get_block_context() != block) {
        return false;
    }

    auto c = cmd->args_count();
    auto v = item.get_values();

    if (c >= 0 && static_cast<size_t>(c) != v.size()) {
        Logger::instance()->error("%s have %d arguments, except %d",
            item.get_name().c_str(), c, v.size());
        return false;
    }

    int rc = cmd->execute(v);

    if (rc == ERROR_COMMAND) {
        return false;
    }

    if (rc != NULL_BLOCK) {    // block command
        for (auto& c : item.get_items()) {
            if (!process(c, rc)) {
                return false;
            }
        }

        if (!cmd->post_execute()) {
            return false;
        }
    }

    return true;
}

bool ConfParser::open(const char *pathname) {
    if (conf_file != nullptr) {
        delete conf_file;
    }
    conf_file = new File(pathname);
    return conf_file->open(O_RDONLY);
}

bool ConfParser::parse_item(ConfItem *parent) {
    ConfItem *item = nullptr;

    while (1) {
        switch (next_token()) {
        case STATE_OK:
            buf[len] = '\0';
            item = parent->gen_sub_item(buf);
            parse_value(item);
            break;
        case STATE_ERROR:
            return false;
        case STATE_END:
            if (len == 0) {
                return false;
            }
            buf[len] = '\0';
            item = parent->gen_sub_item(buf);
            parse_value(item);
            break;
        case STATE_FINISTH:
            return len == 0 && parent == root;
        case STATE_BLOCK_START:
            if (len == 0) {
                return false;
            }
            buf[len] = '\0';
            item = parent->gen_sub_item(buf);
            if (!parse_item(item)) {
                return false;
            }
            break;
        case STATE_BLOCK_END:   // must end with '; }' or keep block empty
            return len == 0;
        }
    }

    return true;
}

bool ConfParser::parse_value(ConfItem* item) {
    while (1) {
        switch (next_token()) {
        case STATE_OK:
            buf[len] = '\0';
            item->push_value(buf);
            break;
        case STATE_END:
            if (len != 0) {
                buf[len] = '\0';
                item->push_value(buf);
            }
            return true;
        case STATE_BLOCK_START:
            if (!parse_item(item)) {
                return false;
            }
            return true;
        case STATE_ERROR:
        case STATE_FINISTH:
        case STATE_BLOCK_END:
            return false;
        }
    }

    return true;
}

int ConfParser::next_token() {
    char ch;

    len = 0;

    while (1) {
        // Todo return -1
        if (conf_file->read(&ch, 1) == 0) {
            if (len != 0) {
                return STATE_ERROR;
            }
            return STATE_FINISTH;
        }

        switch (ch) {
        case '\n':
        case '\r':
            if (len != 0) {
                return STATE_ERROR;
            }
            break;
        case '\t':
        case ' ':
            if (len != 0) {
                return STATE_OK;
            }
            break;
        case ';':
            return STATE_END;
        case '{':
            return STATE_BLOCK_START;
        case '}':
            if (len != 0) {
                return STATE_ERROR;
            }
            return STATE_BLOCK_END;
        default:
            buf[len++] = ch;
        }
    }
}

}

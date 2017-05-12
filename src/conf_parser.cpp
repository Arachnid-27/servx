#include "conf_parser.h"

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
    if (!parse(root)) {
        return false;
    }

    for (auto& c : root->get_items()) {
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

bool ConfParser::parse(ConfItem *parent) {
    ConfItem *item = nullptr;

    while (1) {
        switch (next_token()) {
        case STATE_OK:
            buf[len] = '\0';
            if (item == nullptr) {
                item = parent->gen_sub_item(buf);
            } else {
                item->push_value(buf);
            }
            break;
        case STATE_ERROR:
            return false;
        case STATE_END:
            if (item == nullptr) {
                if (len != 0) {
                    item = parent->gen_sub_item(buf);
                    break;
                }
                return false;   // only ';'
            }

            if (len != 0) {         // end with '[token];' && do nothing if end with '[token] ;'
                buf[len] = '\0';
                item->push_value(buf);
            }

            item = nullptr;
            break;
        case STATE_FINISTH:
            if (item != nullptr || parent != root) {
                // Todo delete item;
                return false;
            }

            return true;
        case STATE_BLOCK_START:
            if (item == nullptr) {  // do nothing if start with '[token] {'
                if (len != 0) {     // start with '[token]{'
                    buf[len] = '\0';
                    item = parent->gen_sub_item(buf);
                } else {            // start with only '{'
                    return false;
                }
            }

            if (!parse(item)) {
                return false;
            }

            item = nullptr;
            break;
        case STATE_BLOCK_END:   // must end with '; }' or keep block empty
            if (parent == root || item != nullptr) {
                // Todo delete item;
                return false;
            }

            return true;
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

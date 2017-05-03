#include "conf.h"


ConfParser* ConfParser::parser = new ConfParser;


bool ConfParser::parse() {
    if (conf_file == nullptr) {
        return false;
    }

    root = new ConfItem("");
    if (!parse(root)) {
        return false;
    }

    auto manager = ModuleManager::instance();

    for (auto c : root->get_items()) {
        if (!execute(c, *manager)) {
            return false;
        }
    }

    return true;
}


bool ConfParser::execute(const ConfItem& item, const ModuleManager& manager) {
    auto cmd = manager.find_command(item.get_name());
    if (cmd == nullptr) {
        return false;
    }

    auto c = cmd->args_count();
    auto v = item.get_values();

    if (c >= 0  && static_cast<size_t>(c) != v.size()) {
        return false;
    }

    if (!cmd->execute(v)) {
        return false;
    }

    auto items = item.get_items();

    if (!items.empty()) {    // block command
        for (auto c : items) {
            if (!execute(c, manager)) {
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
    return conf_file->open(OPEN_MODE_RDONLY);
}


bool ConfParser::parse(ConfItem *parent) {
    ConfItem *item = nullptr;

    while (1) {
        switch (next_token()) {
        case STATE_OK:
            buf[len] = '\0';
            if (item == nullptr) {
                item = new ConfItem(buf);
            } else {
                item->push_value(buf);
            }
            break;
        case STATE_ERROR:
            return false;
        case STATE_END:
            if (item == nullptr) {
                if (len != 0) {
                    item = new ConfItem(buf);
                    break;
                }
                return false;   // only ';'
            }

            if (len != 0) {         // end with '[token];' && do nothing if end with '[token] ;'
                buf[len] = '\0';
                item->push_value(buf);
            }

            parent->push_sub_item(std::move(*item));
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
                    item = new ConfItem(buf);
                } else {            // start with only '{'
                    return false;
                }
            }

            if (!parse(item)) {
                return false;
            }

            parent->push_sub_item(std::move(*item));
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
        ch = conf_file->read();
        switch (ch) {
        case EOF:
            if (len != 0) {
                return STATE_ERROR;
            }
            return STATE_FINISTH;
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

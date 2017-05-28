#include "conf_parser.h"

#include "core.h"
#include "io.h"
#include "logger.h"
#include "module_manager.h"

namespace servx {

ConfParser* ConfParser::parser = new ConfParser;

bool ConfParser::parse() {
    if (conf_file == nullptr) {
        return false;
    }

    if (!parse_item(root.get())) {
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
    auto cmd = ModuleManager::instance()
        ->find_command(block, item.get_name());

    if (cmd == nullptr) {
        Logger::instance()->error("can not find command %s in block %d",
            item.get_name().c_str(), block);
        return false;
    }

    if (cmd->get_block_context() != block) {
        Logger::instance()->error("block context error for %s",
            item.get_name().c_str());
        return false;
    }

    auto c = cmd->args_count();
    auto v = item.get_values();

    if (c >= 0 && static_cast<size_t>(c) != v.size()) {
        Logger::instance()->error("%s have %d arguments, except %d",
            item.get_name().c_str(), v.size(), c);
        return false;
    }

    int rc = cmd->execute(v);

    if (rc == SERVX_ERROR) {
        Logger::instance()->error("execute %s error", item.get_name().c_str());
        return false;
    }

    if (rc != NULL_BLOCK) {    // block command
        for (auto& c : item.get_items()) {
            if (!process(c, rc)) {
                return false;
            }
        }

        if (!cmd->post_execute()) {
            Logger::instance()->error("post execute %s error", item.get_name().c_str());
            return false;
        }
    }

    return true;
}

bool ConfParser::open(const char *pathname) {
    conf_file = std::unique_ptr<File>(new File(pathname));
    root = std::unique_ptr<ConfItem>(new ConfItem(""));
    return conf_file->open(O_RDONLY);
}

bool ConfParser::parse_item(ConfItem *parent) {
    ConfItem *item = nullptr;

    while (1) {
        switch (next_token()) {
        case STATE_OK:
            item = parent->gen_sub_item(buf);
            if (!parse_value(item)) {
                return false;
            }
            break;
        case STATE_ERROR:
            return false;
        case STATE_END: // [token];
            if (*buf == '\0') { // only ';'
                return false;
            }
            item = parent->gen_sub_item(buf);
            break;
        case STATE_FINISTH:
            return parent == root.get();
        case STATE_BLOCK_START:
            if (*buf == '\0') { // only '{'
                return false;
            }
            item = parent->gen_sub_item(buf);
            if (!parse_item(item)) {
                return false;
            }
            break;
        case STATE_BLOCK_END:   // must end with '; }' or keep block empty
            return true;
        }
    }

    return true;
}

bool ConfParser::parse_value(ConfItem* item) {
    while (1) {
        switch (next_token()) {
        case STATE_OK:
            item->push_value(buf);
            break;
        case STATE_END:
            if (*buf != '\0') {
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
    int rc;
    char ch;
    uint16_t len = 0;

    while (1) {
        rc = io_read(conf_file->get_fd(), &ch, 1);
        switch (rc) {
        case 1:
            break;
        case 0:
            if (len == 0) {
                return STATE_FINISTH;
            }
            // fall
        default:
            return STATE_ERROR;
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
                buf[len] = '\0';
                return STATE_OK;
            }
            break;
        case ';':
            buf[len] = '\0';
            return STATE_END;
        case '{':
            buf[len] = '\0';
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

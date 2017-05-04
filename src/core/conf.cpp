#include "conf.h"

namespace servx {

ConfItem* ConfItem::gen_sub_item(const char* s) {
    items.emplace_back(s);
    return &items.back();
}

}

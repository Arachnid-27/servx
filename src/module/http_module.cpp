#include "http_module.h"

#include "module_manager.h"
#include "listener.h"

namespace servx {

int MainHttpModule::http_handler(command_vals_t v) {
    return HTTP_BLOCK;
}

int MainHttpModule::server_handler(command_vals_t v) {
    conf.servers.emplace_back(new Server());
    return SERVER_BLOCK;
}

int MainHttpModule::location_handler(command_vals_t v) {
    if (v[0][0] != '/') {
        // Todo = ^~
        return ERROR_COMMAND;
    }

    if (!conf.servers.back()->push_location(v[0], false)) {
        return ERROR_COMMAND;
    }

    return LOCATION_BLOCK;
}

int MainHttpModule::address_handler(command_vals_t v) {
    if (addr != nullptr) {
        return ERROR_COMMAND;
    }

    if (v.size() != 0) {
        if (v.size() == 1 && v[0] == "default") {
            default_server = true;
        } else {
            return ERROR_COMMAND;
        }
    }

    addr = std::make_shared<IPAddress>();
    return ADDRESS_BLOCK;
}

int MainHttpModule::server_name_handler(command_vals_t v) {
    auto &server = conf.servers.back();
    for (auto &name : v) {
        server->push_server_name(name);
    }
    return NULL_BLOCK;
}

int MainHttpModule::addr_handler(command_vals_t v) {
    if (!addr->set_addr(v[0])) {
        return ERROR_COMMAND;
    }
    return NULL_BLOCK;
}

int MainHttpModule::port_handler(command_vals_t v) {
    return set_address_value(v, &IPAddress::set_port);
}

int MainHttpModule::backlog_handler(command_vals_t v) {
    return set_address_value(v, &IPAddress::set_backlog);
}

int MainHttpModule::send_buf_handler(command_vals_t v) {
    return set_address_value(v, &IPAddress::set_send_buf);
}

int MainHttpModule::recv_buf_handler(command_vals_t v) {
    return set_address_value(v, &IPAddress::set_recv_buf);
}

int MainHttpModule::reuseport_handler(command_vals_t v) {
    addr->set_reuseport(true);
    return NULL_BLOCK;
}

bool MainHttpModule::http_post_handler() {
    conf.servers.shrink_to_fit();
    return true;
}

bool MainHttpModule::server_post_handler() {
    return true;
}

bool MainHttpModule::address_post_handler() {
    Listener::instance()->push_address(
        addr, conf.servers.back(), default_server);
    addr = nullptr;
    default_server = false;
    return true;
}

inline int MainHttpModule::set_address_value(command_vals_t v,
                                             address_setter setter) {
    int val = atoi(v[0].c_str());
    if (val <= 0) {
        return ERROR_COMMAND;
    }
    ((*addr).*setter)(val);

    return NULL_BLOCK;
}

}

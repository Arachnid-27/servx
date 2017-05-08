#include "http_module.h"

#include "module_manager.h"
#include "listener.h"

namespace servx {

int MainHttpModule::http_handler(command_vals_t v) {
    return HTTP_BLOCK;
}

int MainHttpModule::server_handler(command_vals_t v) {
    conf.servers.push_back(Server());
    return SERVER_BLOCK;
}

int MainHttpModule::location_handler(command_vals_t v) {
    conf.servers.back().push_location(new Location(v[0]));
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

    addr = new IPAddress();
    return ADDRESS_BLOCK;
}

int MainHttpModule::server_name_handler(command_vals_t v) {
    Server& server = conf.servers.back();
    for (auto& s : v) {
        server.push_server_name(s);
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
    return true;
}

bool MainHttpModule::server_post_handler() {
    return true;
}

bool MainHttpModule::address_post_handler() {
    Listener::instance()->push_address(
        addr, &conf.servers.back(), default_server);
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
    (addr->*setter)(val);

    return NULL_BLOCK;
}

}

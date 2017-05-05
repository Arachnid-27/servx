#include "http_module.h"

namespace servx {

int MainHttpModule::http_handler(command_vals_t v) {
    return HTTP_BLOCK;
}

int MainHttpModule::server_handler(command_vals_t v) {
    conf.servers.push_back(Server());
    return SERVER_BLOCK;
}

bool MainHttpModule::server_post_handler() {
    return true;
}

int MainHttpModule::location_handler(command_vals_t v) {
    conf.servers.back().push_location(Location(v[0]));
    return LOCATION_BLOCK;
}

int MainHttpModule::address_handler(command_vals_t v) {
    Server& server = conf.servers.back();
    server.new_address();
    return ADDRESS_BLOCK;
}

int MainHttpModule::addr_handler(command_vals_t v) {
    Server& server = conf.servers.back();
    IPAddress& address = server.get_last_address();

    if (!address.set_addr(v[0])) {
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
    Server& server = conf.servers.back();
    IPAddress& address = server.get_last_address();
    address.set_reuseport(true);

    return NULL_BLOCK;
}

inline int MainHttpModule::set_address_value(command_vals_t& v,
                                             address_setter setter) {
    Server& server = conf.servers.back();
    IPAddress& address = server.get_last_address();
    int val = atoi(v[0].c_str());
    if (val <= 0) {
        return ERROR_COMMAND;
    }
    (address.*setter)(val);

    return NULL_BLOCK;
}

}

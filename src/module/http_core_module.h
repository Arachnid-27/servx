#ifndef _HTTP_CORE_MODULE_
#define _HTTP_CORE_MODULE_

#include "http_module.h"
#include "server.h"

namespace servx {

struct HttpCoreMainConf: public ModuleConf {
    std::vector<std::unique_ptr<Server>> servers;
};

class HttpCoreModule
    : public HttpModuleWithConf<HttpCoreMainConf,
                                HttpCoreSrvConf,
                                HttpCoreLocConf,
                                HTTP_CORE_MODULE> {
public:
    HttpCoreModule(): HttpModuleWithConf(
        {
            new Command(CORE_BLOCK,
                        "http",
                        lambda_handler(http_handler), 0,
                        lambda_post_handler(http_post_handler)),
            new Command(HTTP_BLOCK,
                        "server",
                        lambda_handler(server_handler), 0,
                        lambda_post_handler(server_post_handler)),
            new Command(SERVER_BLOCK,
                        "location",
                        lambda_handler(location_handler), 1,
                        lambda_post_handler(location_post_handler)),
            new Command(SERVER_BLOCK,
                        "address",
                        lambda_handler(address_handler), -1,
                        lambda_post_handler(address_post_handler)),
            new Command(SERVER_BLOCK,
                        "server_name",
                        lambda_handler(server_name_handler), -1),
            new Command(SERVER_BLOCK,
                        "client_header_timeout",
                        lambda_handler(client_header_timeout_handler), 1),
            new Command(SERVER_BLOCK,
                        "client_body_tiemout",
                        lambda_handler(client_body_timeout_handler), 1),
            new Command(SERVER_BLOCK,
                        "client_header_buffer_size",
                        lambda_handler(client_header_buffer_size_handler),
                        1),
            new Command(SERVER_BLOCK,
                        "client_body_buffer_size",
                        lambda_handler(client_body_buffer_size_handler),
                        1),
            new Command(ADDRESS_BLOCK,
                        "addr",
                        lambda_handler(addr_handler), 1),
            new Command(ADDRESS_BLOCK,
                        "port",
                        lambda_handler(port_handler), 1),
            new Command(ADDRESS_BLOCK,
                        "backlog",
                        lambda_handler(backlog_handler), 1),
            new Command(ADDRESS_BLOCK,
                        "send_buf",
                        lambda_handler(send_buf_handler), 1),
            new Command(ADDRESS_BLOCK,
                        "recv_buf",
                        lambda_handler(recv_buf_handler), 1),
            new Command(LOCATION_BLOCK,
                        "client_max_body_size",
                        lambda_handler(client_max_body_size_handler), 1),
            new Command(LOCATION_BLOCK,
                        "root",
                        lambda_handler(root_handler), 1),
            new Command(LOCATION_BLOCK,
                        "sendfile",
                        lambda_handler(sendfile_handler), 1)
        }) {}

    int http_handler(command_vals_t v);
    int server_handler(command_vals_t v);
    int location_handler(command_vals_t v);
    int address_handler(command_vals_t v);

    int server_name_handler(command_vals_t v);

    int client_header_timeout_handler(command_vals_t v) {
        return set_conf_int<srv_conf_t,
            &srv_conf_t::client_header_timeout>(
            srv->get_core_conf(), v[0]);
    }

    int client_body_timeout_handler(command_vals_t v) {
        return set_conf_int<srv_conf_t,
            &srv_conf_t::client_body_timeout>(
            srv->get_core_conf(), v[0]);
    }

    int client_header_buffer_size_handler(command_vals_t v) {
        return set_conf_int<srv_conf_t,
            &srv_conf_t::client_header_buffer_size>(
            srv->get_core_conf(), v[0]);
    }

    int client_body_buffer_size_handler(command_vals_t v) {
        return set_conf_int<srv_conf_t,
            &srv_conf_t::client_body_buffer_size>(
            srv->get_core_conf(), v[0]);
    }

    int addr_handler(command_vals_t v);
    int port_handler(command_vals_t v);

    int backlog_handler(command_vals_t v) {
        return set_conf_int<TcpSocket, &TcpSocket::set_backlog>(
            tcp_socket.get(), v[0]);
    }

    int send_buf_handler(command_vals_t v) {
        return set_conf_int<TcpSocket, &TcpSocket::set_send_buf>(
            tcp_socket.get(), v[0]);
    }

    int recv_buf_handler(command_vals_t v) {
        return set_conf_int<TcpSocket, &TcpSocket::set_recv_buf>(
            tcp_socket.get(), v[0]);
    }

    int root_handler(command_vals_t v);
    int sendfile_handler(command_vals_t v);

    int client_max_body_size_handler(command_vals_t v) {
        return set_conf_int<loc_conf_t,
            &loc_conf_t::client_max_body_size>(
            loc->get_core_conf(), v[0]);
    }

    bool http_post_handler();
    bool server_post_handler();
    bool address_post_handler();
    bool location_post_handler();

    static Location* get_cur_location() { return loc.get(); }
    static Server* get_cur_server() { return srv.get(); }

private:
    // temp variables
    std::shared_ptr<TcpSocket> tcp_socket;
    std::string addr;
    std::string port;
    bool default_server;

    static std::unique_ptr<Location> loc;
    static std::unique_ptr<Server> srv;
};

}

#endif

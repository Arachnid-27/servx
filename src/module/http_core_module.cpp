#include "http_core_module.h"

#include "http_listening.h"
#include "http_phase.h"
#include "module_manager.h"
#include "listener.h"
#include "logger.h"

namespace servx {

std::unique_ptr<Location> HttpCoreModule::loc = nullptr;
std::unique_ptr<Server> HttpCoreModule::srv = nullptr;

int HttpCoreModule::http_handler(command_vals_t v) {
    return HTTP_BLOCK;
}

int HttpCoreModule::server_handler(command_vals_t v) {
    srv = std::unique_ptr<Server>(new Server());
    auto cf = srv->get_core_conf();
    cf->client_header_timeout = 60000;
    cf->client_body_timeout = 60000;
    cf->client_header_buffer_size = 4096;
    cf->client_body_buffer_size = 4096;
    return SERVER_BLOCK;
}

int HttpCoreModule::location_handler(command_vals_t v) {
    if (v[0][0] != '/') {
        // Todo = ^~
        return SERVX_ERROR;
    }

    loc = std::unique_ptr<Location>(new Location(v[0]));
    auto cf = loc->get_core_conf();
    cf->client_max_body_size = 4096;
    return LOCATION_BLOCK;
}

int HttpCoreModule::address_handler(command_vals_t v) {
    addr = "*";
    port = "80";
    default_server = false;

    if (v.size() != 0) {
        if (v.size() == 1 && v[0] == "default") {
            default_server = true;
        } else {
            Logger::instance()->error("can not parse %s", v[0].c_str());
            return SERVX_ERROR;
        }
    }

    tcp_socket = std::unique_ptr<TcpSocket>(new TcpSocket());
    return ADDRESS_BLOCK;
}

int HttpCoreModule::server_name_handler(command_vals_t v) {
    for (auto &name : v) {
        srv->push_server_name(name);
    }
    return NULL_BLOCK;
}

int HttpCoreModule::addr_handler(command_vals_t v) {
    addr = v[0];
    return NULL_BLOCK;
}

int HttpCoreModule::port_handler(command_vals_t v) {
    port = v[0];
    return NULL_BLOCK;
}

int HttpCoreModule::root_handler(command_vals_t v) {
    if (v[0].back() == '/') {
        loc->set_root(std::string(v[0].begin(), v[0].end() - 1));
    } else {
        loc->set_root(v[0]);
    }

    return NULL_BLOCK;
}

int HttpCoreModule::sendfile_handler(command_vals_t v) {
    if (v[0] == "on") {
        loc->set_send_file(true);
    }
    return NULL_BLOCK;
}

bool HttpCoreModule::http_post_handler() {
    conf->servers.shrink_to_fit();

    if (!ModuleManager::instance()->for_each([](Module* module) {
            if (module->get_type() == HTTP_MODULE) {
                return static_cast<HttpModule*>(module)->post_configuration();
            }
            return true;
        })) {
        return false;
    }

    HttpPhaseRunner::instance()->init();
    return true;
}

bool HttpCoreModule::server_post_handler() {
    conf->servers.push_back(std::move(srv));
    return true;
}

bool HttpCoreModule::address_post_handler() {
    if (!tcp_socket->init_addr(addr, port)) {
        return false;
    }

    auto lst = Listener::instance()->push_address(std::move(tcp_socket));
    if (lst == nullptr) {
        return false;
    }

    HttpListening *hl = lst->get_context<HttpListening>();
    if (hl == nullptr) {
        hl = new HttpListening;
        lst->set_context(hl);
        lst->set_handler(HttpListening::init_connection);
    }

    return hl->push_server(srv.get(), default_server);
}

bool HttpCoreModule::location_post_handler() {
    if (!srv->push_location(loc.release(), false)) {
        Logger::instance()->error("location duplicate");
        return false;
    }
    return true;
}

};

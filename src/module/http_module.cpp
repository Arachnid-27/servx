#include "http_module.h"

#include "http_listening.h"
#include "http_phase.h"
#include "module_manager.h"
#include "listener.h"
#include "logger.h"

namespace servx {

HttpMainConf HttpMainModule::conf;
HttpMainModule HttpMainModule::instance;
std::vector<Command*> HttpMainModule::commands = {
    new command::Http,
    new command::HServer,
    new command::HLocation,
    new command::Address,
    new command::ServerName,
    new command::Timeout,
    new command::BufferSize,
    new command::Root,
    new command::MaxBodySize,
    new command::Sendfile,
    new command::Addr,
    new command::Port,
    new command::Backlog,
    new command::SendBuf,
    new command::RecvBuf
};

namespace command {

bool Http::post_execute() {
    HttpMainModule::conf.servers.shrink_to_fit();

    auto manager = ModuleManager::instance();
    if (!manager->for_each_http([](HttpModule* module) {
            return module->post_configuration();
        })) {
        return false;
    }

    HttpPhaseRunner::instance()->init();
    return true;
}

bool HServer::execute(const command_args_t& v) {
    HttpMainModule::conf.temp_server.reset(new Server());
    return true;
}

bool HServer::post_execute() {
    HttpMainModule::conf.servers.push_back(
        std::move(HttpMainModule::conf.temp_server));
    return true;
}

bool HLocation::execute(const command_args_t& v) {
    if (v[0][0] != '/') {
        // Todo = ^~
        return false;
    }

    HttpMainModule::conf.temp_location.reset(new Location(v[0]));
    return true;
}

bool HLocation::post_execute() {
    if (!HttpMainModule::conf.temp_server->push_location(
            HttpMainModule::conf.temp_location.release(), false)) {
        Logger::instance()->error("location duplicate");
        return false;
    }
    return true;
}

bool Address::execute(const command_args_t& v) {
    if (v.size() != 0) {
        if (v.size() == 1 && v[0] == "default") {
            HttpMainModule::conf.temp_default = true;
        } else {
            Logger::instance()->error("can not parse %s", v[0].c_str());
            return false;
        }
    } else {
        HttpMainModule::conf.temp_default = false;
    }

    HttpMainModule::conf.temp_socket.reset(new TcpListenSocket());
    return true;
}

bool Address::post_execute() {
    if (HttpMainModule::conf.temp_socket->init_addr() == SERVX_ERROR) {
        Logger::instance()->error("[address]: init address error");
        return false;
    }

    auto lst = Listener::instance()->push_address(
        std::move(HttpMainModule::conf.temp_socket));
    if (lst == nullptr) {
        Logger::instance()->error("[address]: push address error");
        return false;
    }

    HttpListening *hl = lst->get_context<HttpListening>();
    if (hl == nullptr) {
        hl = new HttpListening;
        lst->set_context(hl);
        lst->set_handler(HttpListening::init_connection);
    }

    return hl->push_server(HttpMainModule::conf.temp_server.get(),
        HttpMainModule::conf.temp_default);
}

bool ServerName::execute(const command_args_t& v) {
    for (auto &name : v) {
        HttpMainModule::conf.temp_server->push_server_name(name);
    }
    return true;
}

bool Root::execute(const command_args_t& v) {
    if (v[0].back() == '/') {
        HttpMainModule::conf.temp_location
            ->set_root(std::string(v[0].begin(), v[0].end() - 1));
    } else {
        HttpMainModule::conf.temp_location->set_root(v[0]);
    }

    return true;
}

}

};

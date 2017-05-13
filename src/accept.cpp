#include "accept.h"

#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include "connection_pool.h"
#include "event_module.h"
#include "listener.h"
#include "logger.h"
#include "module_manager.h"
#include "timer.h"

namespace servx {

void accept_event_handler(Listening* lst, Event* ev) {
    if (ev->is_timeout()) {
        // if we get ENFILE or EMFILE
        // it means we can't accept until we release some fds
        // so we will disable accept events and set a timer
        // when the timer expire we enable accept events and try again
        if (!Listener::instance()->enable_all()) {
            Logger::instance()->info("enable all listening fd");
            return;
        }
    }

    bool multi = ModuleManager::instance()
        ->get_conf<MainEventModule>()->multi_accept;
    Connection *conn = ev->get_connection();

    sockaddr sa;
    socklen_t len;
    int fd, err;

    ev->set_ready(false);   //

    while (true) {
        fd = accept4(conn->get_fd(), &sa, &len, SOCK_NONBLOCK);

        if (fd == -1) {
            err = errno;

            if (err == EAGAIN) {
                Logger::instance()->info("accept4() not ready");
                return;
            }

            if (err == ENOSYS) {
                // have no accept4
                return;
            }

            if (err == ECONNABORTED) {
                if (multi) {
                    continue;
                }
                return;
            }

            if (err == EMFILE || err == ENFILE) {
                if (Listener::instance()->disable_all()) {
                    Logger::instance()->info("disable all listening fd");
                    Timer::instance()->add_timer(ev, 500);
                }
            }

            return;
        }

        Connection *conn = ConnectionPool::instance()->get_connection(fd);
        if (conn == nullptr) {
            if (::close(fd) == -1) {
                Logger::instance()->warn("close fd %d failed", fd);
                return;
            }
        }
        conn->set_peer_sockaddr(&sa, len);

        conn->get_write_event()->set_ready(true); // enable write event
        if (lst->get_socket()->is_deferred_accept()) {
            conn->get_read_event()->set_ready(true);
        }

        lst->handle(conn);

        if (!multi) {
            break;
        }
    }
}

}

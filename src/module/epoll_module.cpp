#include "epoll_module.h"


bool EpollModule::init_conf() {
    conf->epoll_events = 512;
    return true;
}


bool EpollModule::init_process() {
    if (conf->epoll_events <= 0) {
        return false;
    }

    ep = epoll_create(1);
    if (ep == -1) {
        return false;
    }

    event_list = new epoll_event[conf->epoll_events];

    return true;
}


bool EpollModule::add_event(Event* ev, int flags) {
    int op, prev;
    epoll_event ee;
    bool active = false;
    Connection *c = ev->get_connection();

    if (ev->is_write_event()) {
        active = c->get_read_event()->is_event_active();
        prev = EPOLLIN | EPOLLRDHUP;
    } else {
        active = c->get_write_event()->is_event_active();
        prev = EPOLLOUT;
    }

    if (active) {
        op = EPOLL_CTL_MOD;
        flags |= prev;
    } else {
        op = EPOLL_CTL_ADD;
    }

    // EPOLLEXCLUSIVE since Linux 4.5.0

    ee.events = flags;
    ee.data.ptr = c;

    if (epoll_ctl(ep, op, c->get_fd(), &ee) == -1) {
        return false;
    }

    ev->set_event_active(true);

    return true;
}


bool EpollModule::del_event(Event* ev, int flags) {
    Connection *c = ev->get_connection();

    if (c->get_fd() == -1) {
        ev->set_event_active(false);
        return true;
    }

    int op, prev;
    epoll_event ee;
    bool active = false;

    if (ev->is_write_event()) {
        active = c->get_read_event()->is_event_active();
        prev = EPOLLIN | EPOLLRDHUP;
    } else {
        active = c->get_write_event()->is_event_active();
        prev = EPOLLOUT;
    }

    if (active) {
        op = EPOLL_CTL_MOD;
        ee.events = prev | flags;
        ee.data.ptr = c;
    } else {
        op = EPOLL_CTL_DEL;
        ee.events = 0;
        ee.data.ptr = NULL;
    }

    if (epoll_ctl(ep, op, c->get_fd(), &ee) == -1) {
        return false;
    }

    ev->set_event_active(false);

    return true;
}


bool EpollModule::add_connection(Connection* c) {
    epoll_event ee;

    ee.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
    ee.data.ptr = &c;

    if (epoll_ctl(ep, EPOLL_CTL_ADD, c->get_fd(), &ee) == -1) {
        return false;
    }

    return true;
}


bool EpollModule::del_connection(Connection* c) {
    if (c->get_fd() != -1) {
        epoll_event ee;

        ee.events = 0;
        ee.data.ptr = NULL;

        if (epoll_ctl(ep, EPOLL_CTL_DEL, c->get_fd(), &ee) == -1) {
            return false;
        }
    }

    c->get_read_event()->set_event_active(false);
    c->get_write_event()->set_event_active(false);

    return true;
}
 

bool EpollModule::process_events() {
    int n = epoll_wait(ep, event_list, conf->epoll_events, -1);

    if (true) { // Todo wake up by SIGALRM
        Clock::instance()->update(); 
    }

    if (n == -1) {
        if (errno == EINTR) {
            if (true) { // Todo wake up by SIGALRM
                return true;
            }
        }

        return false;
    }

    if (n == 0) {
        return false;
    }

    int flags;
    Event *event;
    Connection *c;

    for (int i = 0; i < n; ++i) {
        c = static_cast<Connection*>(event_list[i].data.ptr);

        if (c->get_fd() == -1) {    // Todo handle stale event
            continue;
        }

        flags = event_list[i].events;

        if (flags & (EPOLLERR | EPOLLHUP)) {
            flags |= (EPOLLIN | EPOLLOUT);
        }

        event = c->get_read_event();

        if ((flags & EPOLLIN) && event->is_event_active()) {
            if (flags & EPOLLRDHUP) {
                // Todo
            }

            event->set_ready(true);
            event->handle();
        }

        event = c->get_write_event();

        if ((flags & EPOLLOUT) && event->is_event_active()) {
            if (c->get_fd() == -1) {    // Todo handle stale event
                continue;
            }

            event->set_ready(true);
            event->handle();
        }
    }

    return true;
}

bool EpollModule::epoll_events_handler(command_vals_t v) {
    auto conf = ModuleManager::instance()->get_conf<EpollModule>();

    conf->epoll_events = atoi(v[0].c_str());

    if (conf->epoll_events <= 0) {
        return false;
    }

    return true;
}

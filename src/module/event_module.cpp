#include "event_module.h"


bool MainEventModule::event_handler(command_vals_t v) {
    return true;
}


bool MainEventModule::timer_resolution_handler(command_vals_t v) {
    return true;
}


bool MainEventModule::connections_handler(command_vals_t) {
    return true;
}


bool MainEventModule::use_handler(command_vals_t) {
    return true;
}

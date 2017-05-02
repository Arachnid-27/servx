#ifndef _DAEMON_H_
#define _DAEMON_H_

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstdlib>

bool daemonize();

#endif

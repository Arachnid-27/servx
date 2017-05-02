#ifndef _CORE_H_
#define _CORE_H_

#include <cstdlib>
#include <cstring>
#include <mutex>
#include <chrono>
#include <iostream>
#include <vector>
#include <memory>
#include <queue>
#include <algorithm>

#include "process.h"
#include "daemon.h"
#include "file.h"

class Event;
class Listening;
class Connection;
class Cycle;
class Clock;

#include "module.h"

#include "event.h"
#include "clock.h"
#include "listening.h"
#include "connection.h"
#include "cycle.h"
#include "worker.h"
#include "master.h"

#endif

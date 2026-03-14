
#include "tasks.h"
#include "configs.h"


TaskInfo heartbeatTask = { HEARTBEAT_INTERVAL, 0 };


bool taskReady(TaskInfo* t, unsigned long now) {
  unsigned long elapsed = now - t->lastRun;


  if (elapsed >= t->interval) {
    t->lastRun = now;


    return true;
  }

  return false;
}

void setupTasks() {
  heartbeatTask.lastRun = 0;
}

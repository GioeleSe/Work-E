#pragma once


#include<Arduino.h>

struct TaskInfo {
  unsigned long interval;
  unsigned long lastRun;
};

extern TaskInfo heartbeatTask;

bool taskReady(TaskInfo *t, unsigned long now);

void setupTasks();

/*
 * TimedTask.cpp
 *
 *  Created on: 24 jul 2026
 *      Author: DjSteker
 */

#include "TimedTask.h"
#include "TimerSys.h"

TimedTask *TaskScheduler::tasks[MAX_TIMED_TASKS];
uint8_t TaskScheduler::taskCount = 0;

TimedTask::TimedTask(unsigned long intervl, void (*function)(void))
  : active(true), previous(0), interval(intervl), execute(function) {
}

TimedTask::TimedTask(unsigned long prev, unsigned long intervl, void (*function)(void), bool enable)
  : active(enable), previous(prev), interval(intervl), execute(function) {
}

void TimedTask::reset() {
  previous = SystemTimer::getMicros();
}

void TimedTask::disable() {
  active = false;
}

void TimedTask::enable() {
  active = true;
}

void TimedTask::setInterval(unsigned long intervl) {
  interval = intervl;
}

void TimedTask::check() {
  if (active && (SystemTimer::getMicros() - previous >= interval)) {
    previous = SystemTimer::getMicros();
    if (execute) {
      execute();
    }
  } else if (active && SystemTimer::getMicros() < previous) {
    unsigned long TMr = (4294967295UL - previous);
    if (TMr < interval) {
      previous = interval - TMr;
    } else {
      previous = SystemTimer::getMicros();
      if (execute) {
        execute();
      }
    }
  }
}

bool TimedTask::isDue() {
  if (!active) {
    return false;
  }
  unsigned long now = SystemTimer::getMicros();
  if (now - previous >= interval) {
    previous = now;
    return true;
  }
  if (now < previous) {
    unsigned long elapsed = (0xFFFFFFFF - previous) + now + 1;
    if (elapsed >= interval) {
      previous = now;
      return true;
    }
  }
  return false;
}

void TaskScheduler::registerTask(TimedTask *task) {
  if (taskCount < MAX_TIMED_TASKS) {
    tasks[taskCount++] = task;
  }
}

void TaskScheduler::checkAll() {
  for (uint8_t i = 0; i < taskCount; i++) {
    if (tasks[i]) {
      tasks[i]->check();
    }
  }
}

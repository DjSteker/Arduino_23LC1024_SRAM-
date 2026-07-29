/*
 * TimedTask.h
 *
 *  Created on: 24 jul 2026
 *      Author: DjSteker
 */

#ifndef TIMED_TASK_H
#define TIMED_TASK_H

#include <stdint.h>

#define MAX_TIMED_TASKS 4

class TimedTask {
public:
  bool active;
  unsigned long previous;
  unsigned long interval;
  void (*execute)(void);

  TimedTask(unsigned long intervl, void (*function)(void));
  TimedTask(unsigned long prev, unsigned long intervl, void (*function)(void), bool enable);

  void reset();
  void disable();
  void enable();
  void check();
  void setInterval(unsigned long intervl);
  bool isDue();
};

class TaskScheduler {
private:
  static TimedTask *tasks[MAX_TIMED_TASKS];
  static uint8_t taskCount;
public:
  static void registerTask(TimedTask *task);
  static void checkAll();
};

#endif /* TIMED_TASK_H */

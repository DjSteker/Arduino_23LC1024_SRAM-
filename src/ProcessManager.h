/*
 * ProcessManager.h
 *
 *  Created on: 24 jul 2026
 *      Author: DjSteker
 */

#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "Process.h"

#ifndef MAX_PROCESSES
#define MAX_PROCESSES 4
#endif

class ProcessManager {
private:
  Process *table[MAX_PROCESSES];
  uint8_t next_pid;
  uint8_t getFreeSlot(void);

public:
  ProcessManager();
  void init(void);
  void schedule(void);

  Process *getByIndex(uint8_t idx);
  Process *find(uint8_t pid);

  Process *create(const char *name, void (*entry)(Process *), process_type_t type, process_prio_t prio, uint8_t parent_pid = 0, unsigned long interval = 0, bool enabled = true);

  Process *spawn(void (*entry)(Process *), void (*cleanup)(Process *) = NULL, unsigned long interval = 0, bool enabled = true);

  void attachChild(Process *parent, Process *child);
  void detachChild(Process *child);
  void terminate(uint8_t pid, bool force = false);
  void garbageCollector(void);
  uint8_t count(void);
};

extern ProcessManager PM;

#endif /* PROCESS_MANAGER_H */

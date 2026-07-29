/*
 * Process.h
 *
 *  Created on: 24 jul 2026
 *      Author: DjSteker
 */

#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h>
#include <avr/pgmspace.h>
#include "TimedTask.h"

#define PROCESS_NAME_LEN 12

typedef enum {
  PROC_FREE = 0,
  PROC_READY,
  PROC_RUNNING,
  PROC_SUSPENDED,
  PROC_WAITING,
  PROC_KILL_PENDING,
  PROC_ZOMBIE
} process_state_t;

typedef enum {
  PROC_KERNEL = 0,
  PROC_USER,
  PROC_DAEMON
} process_type_t;

typedef enum {
  PRIO_IDLE = 0,
  PRIO_LOW = 1,
  PRIO_NORMAL = 2,
  PRIO_HIGH = 3,
  PRIO_CRITICAL = 4
} process_prio_t;

class Process {
public:
  uint8_t pid;
  process_state_t state;
  const char *name_P;
  process_type_t type;
  process_prio_t priority;

  uint32_t mem_used;
  uint16_t alloc_count;

  TimedTask *task;

  uint8_t ref_count;
  Process *first_child;
  Process *next_sibling;
  Process *parent;

  void (*entry_point)(Process *current_proc);
  void (*cleanup_callback)(Process *current_proc);

  Process(uint8_t id, const char *name, process_type_t p_type, process_prio_t prio, void (*entry)(Process *), void (*cleanup)(Process *), unsigned long interval = 0, bool enabled = true);
  ~Process();

  void addChild(Process *child);
  void removeChild(Process *child);
  void prepareForDeath(void);
  uint8_t countChildren(void);
};

#endif /* PROCESS_H */

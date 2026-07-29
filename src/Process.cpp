/*
 * Process.cpp
 *
 *  Created on: 24 jul 2026
 *      Author: DjSteker
 */

#include "Process.h"

Process::Process(uint8_t id, const char *name, process_type_t p_type, process_prio_t prio, void (*entry)(Process *), void (*cleanup)(Process *), unsigned long interval, bool enabled) {
  this->pid = id;
  this->name_P = name;
  this->type = p_type;
  this->priority = prio;
  this->state = PROC_READY;
  this->mem_used = 0;
  this->alloc_count = 0;
  this->ref_count = 1;
  this->first_child = NULL;
  this->next_sibling = NULL;
  this->parent = NULL;
  this->entry_point = entry;
  this->cleanup_callback = cleanup;
  this->task = new TimedTask(0, interval, NULL, enabled);
}

Process::~Process() {
  if (this->task) {
    delete this->task;
    this->task = NULL;
  }
}

void Process::addChild(Process *child) {
  if (!child) {
    return;
  }
  if (!this->first_child) {
    this->first_child = child;
  } else {
    Process *temp = this->first_child;
    while (temp->next_sibling) {
      temp = temp->next_sibling;
    }
    temp->next_sibling = child;
  }
  child->ref_count++;
  child->parent = this;
}

void Process::removeChild(Process *child) {
  if (!child || !this->first_child) {
    return;
  }
  if (this->first_child == child) {
    this->first_child = child->next_sibling;
  } else {
    Process *temp = this->first_child;
    while (temp->next_sibling && temp->next_sibling != child) {
      temp = temp->next_sibling;
    }
    if (temp->next_sibling == child) {
      temp->next_sibling = child->next_sibling;
    }
  }
  if (child->ref_count > 0) {
    child->ref_count--;
  }
  child->parent = NULL;
}

void Process::prepareForDeath(void) {
  if (this->cleanup_callback) {
    this->cleanup_callback(this);
  }

  Process *curr = this->first_child;
  while (curr) {
    Process *nxt = curr->next_sibling;
    if (curr->ref_count > 0) {
      curr->ref_count--;
    }
    if (curr->ref_count == 0 && curr->state != PROC_ZOMBIE) {
      curr->state = PROC_KILL_PENDING;
    }
    curr = nxt;
  }
  this->state = PROC_ZOMBIE;
}

uint8_t Process::countChildren(void) {
  uint8_t c = 0;
  Process *curr = this->first_child;
  while (curr) {
    c++;
    curr = curr->next_sibling;
  }
  return c;
}

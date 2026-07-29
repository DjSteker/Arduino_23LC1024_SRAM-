/*
 * TimerSys.h
 *
 *  Created on: 24 jul 2026
 *      Author: DjSteker
 */

#ifndef TIMER_SYS_H
#define TIMER_SYS_H

#include <stdint.h>

class SystemTimer {
public:
  static void init();
  static uint32_t getMillis();
  static unsigned long getMicros();
};

#endif /* TIMER_SYS_H */

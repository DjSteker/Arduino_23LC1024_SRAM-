/*
 * BareMetalRAM.h
 * 
 *  Created on: 24 jul 2026
 *      Author: DjSteker
 */

#ifndef BARE_METAL_RAM_H
#define BARE_METAL_RAM_H

#include <Arduino.h>
#include "SRAMsimple.h"
#include "ExtSRAMHeap.h"
#include "TimerSys.h"
#include "TimedTask.h"
#include "Process.h"
#include "ProcessManager.h"
#include "PageCache.h"

class BareMetalRAM {
public:
  void begin(uint8_t csPin);
};

extern BareMetalRAM BMRAM;

#endif /* BARE_METAL_RAM_H */

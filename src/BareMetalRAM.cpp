/*
 * BareMetalRAM.cpp
 * 
 *  Created on: 24 jul 2026
 *      Author: DjSteker
 */

#include "BareMetalRAM.h"

SRAMsimple sram;
BareMetalRAM BMRAM;

void BareMetalRAM::begin(uint8_t csPin) {
  sram.begin(csPin, SEQ_MODE);
  ExtHeap.init();           // CAMBIADO: ExtSRAM.init() -> ExtHeap.init();
  SystemTimer::init();
  PageCache.init();
  PM.init();
}

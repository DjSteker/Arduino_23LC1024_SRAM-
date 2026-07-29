/*
 * ExtSRAMHeap.h
 *
 *  Created on: 24 jul 2026
 *      Author: DjSteker
 */

#ifndef EXT_SRAM_HEAP_H
#define EXT_SRAM_HEAP_H

#include <Arduino.h>
#include "SRAMsimple.h"

#define SRAM_SIZE 131072UL
#define HEAP_RESERVED_TOP 1024UL
#define HEAP_START_ADDR 0
#define HEAP_TOTAL_SIZE (SRAM_SIZE - HEAP_RESERVED_TOP)
#define HEAP_MAGIC 0xAA

struct BlockHeader {
  uint32_t size;
  uint8_t free;
  uint8_t magic;
  uint8_t owner;
};

#define HEADER_SIZE sizeof(BlockHeader)

class ExtSRAM {
public:
  void init();
  uint32_t malloc(uint16_t size, uint8_t owner_pid = 0);
  void free(uint32_t dataAddr);
  void defrag();
  uint32_t getFree();
  uint32_t getUsed();
  void walk();
};

// CAMBIADO: ExtSRAM ExtSRAM; -> ExtSRAM ExtHeap;
extern ExtSRAM ExtHeap;

#endif /* EXT_SRAM_HEAP_H */

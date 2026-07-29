/*
 * ExtSRAMHeap.cpp
 *
 *  Created on: 24 jul 2026
 *      Author: DjSteker
 */

#include "ExtSRAMHeap.h"

extern SRAMsimple sram;

// CAMBIADO: ExtSRAM ExtSRAM; -> ExtSRAM ExtHeap;
ExtSRAM ExtHeap;

void ExtSRAM::init(void) {
  BlockHeader initial;
  initial.size = HEAP_TOTAL_SIZE - HEADER_SIZE;
  initial.free = 1;
  initial.magic = HEAP_MAGIC;
  initial.owner = 0;
  sram.writeByteArray(HEAP_START_ADDR, (byte *)&initial, HEADER_SIZE);
}

uint32_t ExtSRAM::malloc(uint16_t size, uint8_t owner_pid) {
  if (size == 0) {
    return 0;
  }

  uint32_t curr = HEAP_START_ADDR;
  while (curr < HEAP_START_ADDR + HEAP_TOTAL_SIZE) {
    BlockHeader hdr;
    sram.readByteArray(curr, (byte *)&hdr, HEADER_SIZE);

    if (hdr.magic != HEAP_MAGIC) {
#ifdef BM_DEBUG
      Serial.println(F("[HEAP] ERROR: Corrupcion de memoria!"));
#endif
      return 0;
    }

    if (hdr.free && hdr.size >= size) {
      if (hdr.size > size + HEADER_SIZE + 4) {
        uint32_t nextAddr = curr + HEADER_SIZE + size;
        BlockHeader next;
        next.size = hdr.size - size - HEADER_SIZE;
        next.free = 1;
        next.magic = HEAP_MAGIC;
        next.owner = 0;
        sram.writeByteArray(nextAddr, (byte *)&next, HEADER_SIZE);
        hdr.size = size;
      }

      hdr.free = 0;
      hdr.owner = owner_pid;
      sram.writeByteArray(curr, (byte *)&hdr, HEADER_SIZE);
      return curr + HEADER_SIZE;
    }

    curr += HEADER_SIZE + hdr.size;
  }
  return 0;
}

void ExtSRAM::free(uint32_t dataAddr) {
  if (dataAddr == 0 || dataAddr <= HEAP_START_ADDR + HEADER_SIZE) {
    return;
  }

  uint32_t hdrAddr = dataAddr - HEADER_SIZE;
  BlockHeader hdr;
  sram.readByteArray(hdrAddr, (byte *)&hdr, HEADER_SIZE);

  if (hdr.magic == HEAP_MAGIC && !hdr.free) {
    hdr.free = 1;
    hdr.owner = 0;
    sram.writeByteArray(hdrAddr, (byte *)&hdr, HEADER_SIZE);
  }
}

void ExtSRAM::defrag(void) {
  uint32_t curr = HEAP_START_ADDR;
  while (curr < HEAP_START_ADDR + HEAP_TOTAL_SIZE) {
    BlockHeader hdr;
    sram.readByteArray(curr, (byte *)&hdr, HEADER_SIZE);

    if (hdr.magic != HEAP_MAGIC) {
      break;
    }

    if (hdr.free) {
      uint32_t next = curr + HEADER_SIZE + hdr.size;
      if (next < HEAP_START_ADDR + HEAP_TOTAL_SIZE) {
        BlockHeader nxt;
        sram.readByteArray(next, (byte *)&nxt, HEADER_SIZE);
        if (nxt.magic == HEAP_MAGIC && nxt.free) {
          hdr.size += HEADER_SIZE + nxt.size;
          sram.writeByteArray(curr, (byte *)&hdr, HEADER_SIZE);
          continue;
        }
      }
    }
    curr += HEADER_SIZE + hdr.size;
  }
}

uint32_t ExtSRAM::getFree(void) {
  uint32_t total = 0;
  uint32_t curr = HEAP_START_ADDR;
  while (curr < HEAP_START_ADDR + HEAP_TOTAL_SIZE) {
    BlockHeader hdr;
    sram.readByteArray(curr, (byte *)&hdr, HEADER_SIZE);

    if (hdr.magic != HEAP_MAGIC) {
      break;
    }

    if (hdr.free) {
      total += hdr.size;
    }
    curr += HEADER_SIZE + hdr.size;
  }
  return total;
}

uint32_t ExtSRAM::getUsed(void) {
  return HEAP_TOTAL_SIZE - HEADER_SIZE - getFree();
}

void ExtSRAM::walk(void) {
#ifdef BM_DEBUG
  Serial.println(F("--- HEAP WALK ---"));
  uint32_t curr = HEAP_START_ADDR;
  int blk = 0;
  while (curr < HEAP_START_ADDR + HEAP_TOTAL_SIZE) {
    BlockHeader hdr;
    sram.readByteArray(curr, (byte *)&hdr, HEADER_SIZE);

    if (hdr.magic != HEAP_MAGIC) {
      Serial.println(F("Bloque corrupto. Abortando."));
      break;
    }

    Serial.print(F("Blk "));
    Serial.print(blk++);
    Serial.print(F(" @0x"));
    Serial.print(curr, HEX);

    if (hdr.free) {
      Serial.print(F(" FREE "));
    } else {
      Serial.print(F(" USED "));
    }

    Serial.print(F("sz="));
    Serial.print(hdr.size);
    Serial.print(F(" own="));
    Serial.println(hdr.owner);

    curr += HEADER_SIZE + hdr.size;
  }
  Serial.println(F("-----------------"));
#endif
}

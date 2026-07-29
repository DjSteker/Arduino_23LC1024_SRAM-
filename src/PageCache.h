/*
 * PageCache.h
 * 
 * Sistema de Paginacion y Cache para BareMetalRAM
 * 
 * Caracteristicas:
 *   - Paginacion: Paginas de 256 bytes, 512 paginas fisicas (128KB)
 *   - Cache: 4-way set associative, 512 bytes en SRAM interna
 *   - Write-through policy con dirty bit tracking
 *   - Reemplazo LRU mediante reloj logico interno (sin getMicros)
 *   - Proteccion entre procesos por PID
 * 
 *  Created on: 24 jul 2026
 *      Author: DjSteker
 */

#ifndef PAGE_CACHE_H
#define PAGE_CACHE_H

#include <Arduino.h>
#include <stdint.h>
#include "Process.h"
#include "ProcessManager.h"

// ============================================================================
// CONFIGURACION DE PAGINACION
// ============================================================================

#ifndef PAGE_SIZE
#define PAGE_SIZE 256UL
#endif

#ifndef MAX_PHYSICAL_PAGES
#define MAX_PHYSICAL_PAGES 512
#endif

#ifndef MAX_PAGES_PER_PROC
#define MAX_PAGES_PER_PROC 16
#endif

// ============================================================================
// CONFIGURACION DE CACHE
// ============================================================================

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 32UL
#endif

#ifndef CACHE_NUM_WAYS
#define CACHE_NUM_WAYS 4
#endif

#ifndef CACHE_NUM_SETS
#define CACHE_NUM_SETS 4
#endif

// Derivados
#define CACHE_TOTAL_LINES (CACHE_NUM_SETS * CACHE_NUM_WAYS)
#define CACHE_TOTAL_SIZE (CACHE_LINE_SIZE * CACHE_TOTAL_LINES)
#define LINES_PER_PAGE (PAGE_SIZE / CACHE_LINE_SIZE)

// ============================================================================
// ESTADOS Y ESTRUCTURAS
// ============================================================================

typedef enum {
  PAGE_FREE = 0,
  PAGE_ALLOCATED,
  PAGE_MAPPED
} page_state_t;

// Entrada en la tabla de paginas de un proceso
struct PageTableEntry {
  uint16_t physical_page;
  uint8_t valid;
  uint8_t dirty;
  uint16_t access_count;
  uint32_t last_access;
};

// Descriptor de pagina fisica (Frame Table)
struct PhysicalPageDescriptor {
  uint8_t state;
  uint8_t owner_pid;
  uint16_t ref_count;
  uint32_t access_time;
};

// Linea de cache
struct CacheLine {
  uint32_t tag;
  uint8_t valid;
  uint8_t dirty;
  uint8_t data[CACHE_LINE_SIZE];
  uint32_t last_access;
};

// Estadisticas de la cache
struct CacheStats {
  uint32_t hits;
  uint32_t misses;
  uint32_t read_hits;
  uint32_t read_misses;
  uint32_t write_hits;
  uint32_t write_misses;
  uint32_t evictions;
  uint32_t writebacks;

  uint8_t getHitRate() const {
    uint32_t total = hits + misses;
    if (total > 0) {
      return (uint8_t)((hits * 100UL) / total);
    } else {
      return 0;
    }
  }
};

// ============================================================================
// CLASE PRINCIPAL
// ============================================================================

class PageCacheManager {
private:
  PhysicalPageDescriptor phys_pages[MAX_PHYSICAL_PAGES];
  PageTableEntry page_table[MAX_PROCESSES][MAX_PAGES_PER_PROC];
  CacheLine cache_sets[CACHE_NUM_SETS][CACHE_NUM_WAYS];
  CacheStats stats;
  uint32_t logical_clock;

  int8_t findFreeLine(uint8_t set);
  int8_t findLRULine(uint8_t set);
  void writeLineToSRAM(uint32_t addr, CacheLine *line);
  void readLineFromSRAM(uint32_t addr, CacheLine *line);
  void touchPage(uint8_t pid, uint16_t logical_page);
  uint32_t tick();

  int16_t findFreePhysicalPage();
  int16_t findVictimPage();

public:
  PageCacheManager();

  void init();

  int16_t allocatePhysicalPage(uint8_t owner_pid);
  void freePhysicalPage(uint16_t phys_page);
  const PhysicalPageDescriptor *getPageInfo(uint16_t phys_page) const;

  bool mapPage(uint8_t pid, uint16_t logical_page, uint16_t physical_page);
  void unmapPage(uint8_t pid, uint16_t logical_page);
  uint32_t translateAddress(uint8_t pid, uint32_t logical_addr) const;
  PageTableEntry *getPageEntry(uint8_t pid, uint16_t logical_page);
  bool isValidAddress(uint8_t pid, uint32_t logical_addr) const;

  uint8_t cacheReadByte(uint8_t pid, uint32_t logical_addr);
  void cacheWriteByte(uint8_t pid, uint32_t logical_addr, uint8_t data);
  void cacheReadArray(uint8_t pid, uint32_t logical_addr, uint8_t *buffer, uint16_t len);
  void cacheWriteArray(uint8_t pid, uint32_t logical_addr, const uint8_t *data, uint16_t len);

  void cacheWriteInt(uint8_t pid, uint32_t addr, int16_t data);
  int16_t cacheReadInt(uint8_t pid, uint32_t addr);
  void cacheWriteUInt(uint8_t pid, uint32_t addr, uint16_t data);
  uint16_t cacheReadUInt(uint8_t pid, uint32_t addr);
  void cacheWriteLong(uint8_t pid, uint32_t addr, int32_t data);
  int32_t cacheReadLong(uint8_t pid, uint32_t addr);
  void cacheWriteULong(uint8_t pid, uint32_t addr, uint32_t data);
  uint32_t cacheReadULong(uint8_t pid, uint32_t addr);
  void cacheWriteFloat(uint8_t pid, uint32_t addr, float data);
  float cacheReadFloat(uint8_t pid, uint32_t addr);

  void invalidateProcessCache(uint8_t pid);
  void invalidateLine(uint32_t physical_addr);
  void flushCache();
  void flushProcessCache(uint8_t pid);
  void invalidateAll();

  uint32_t pagedMalloc(uint8_t pid, uint16_t size);
  void pagedFree(uint8_t pid, uint32_t logical_addr);
  uint32_t pagedRealloc(uint8_t pid, uint32_t old_addr, uint16_t new_size);

  CacheStats getStats() const;
  void resetStats();
  uint16_t getFreePages() const;
  uint16_t getUsedPages() const;
  uint16_t getProcessPages(uint8_t pid) const;
  uint32_t getProcessMemory(uint8_t pid) const;

  void printCacheState();
  void printPageTable(uint8_t pid);
  void printPhysicalPageMap();
  void printSummary();
};

extern PageCacheManager PageCache;

#endif /* PAGE_CACHE_H */

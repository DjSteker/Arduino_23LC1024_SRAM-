/*
 * PageCache.cpp
 * 
 * Implementación del sistema de paginación y caché
 * 
 *  Created on: 24 jul 2026
 *      Author: DjSteker
 */

#include "PageCache.h"
#include "SRAMsimple.h"
#include <string.h>

extern SRAMsimple sram;

PageCacheManager PageCache;

// ============================================================================
// CONSTRUCTOR
// ============================================================================

PageCacheManager::PageCacheManager() {
  memset(phys_pages, 0, sizeof(phys_pages));
  memset(page_table, 0, sizeof(page_table));
  memset(cache_sets, 0, sizeof(cache_sets));
  memset(&stats, 0, sizeof(stats));
  logical_clock = 0;
}

// ============================================================================
// INICIALIZACION
// ============================================================================

void PageCacheManager::init() {
  logical_clock = 0;

  for (uint16_t i = 0; i < MAX_PHYSICAL_PAGES; i++) {
    phys_pages[i].state = PAGE_FREE;
    phys_pages[i].owner_pid = 0;
    phys_pages[i].ref_count = 0;
    phys_pages[i].access_time = 0;
  }

  for (uint8_t p = 0; p < MAX_PROCESSES; p++) {
    for (uint16_t i = 0; i < MAX_PAGES_PER_PROC; i++) {
      page_table[p][i].physical_page = 0;
      page_table[p][i].valid = 0;
      page_table[p][i].dirty = 0;
      page_table[p][i].access_count = 0;
      page_table[p][i].last_access = 0;
    }
  }

  for (uint8_t s = 0; s < CACHE_NUM_SETS; s++) {
    for (uint8_t w = 0; w < CACHE_NUM_WAYS; w++) {
      cache_sets[s][w].valid = 0;
      cache_sets[s][w].dirty = 0;
      cache_sets[s][w].tag = 0;
      cache_sets[s][w].last_access = 0;
      memset(cache_sets[s][w].data, 0, CACHE_LINE_SIZE);
    }
  }

#ifdef BM_DEBUG
  Serial.println(F("[PageCache] Sistema inicializado"));
  Serial.print(F("  Paginacion: "));
  Serial.print(PAGE_SIZE);
  Serial.print(F("B/pag, "));
  Serial.print(MAX_PHYSICAL_PAGES);
  Serial.println(F(" paginas fisicas"));
  Serial.print(F("  Cache L1: "));
  Serial.print(CACHE_TOTAL_SIZE);
  Serial.print(F("B ("));
  Serial.print(CACHE_NUM_SETS);
  Serial.print(F(" sets x "));
  Serial.print(CACHE_NUM_WAYS);
  Serial.print(F(" ways x "));
  Serial.print(CACHE_LINE_SIZE);
  Serial.println(F("B/line)"));
  Serial.print(F("  LRU basado en reloj logico interno"));
#endif
}

// ============================================================================
// METODOS PRIVADOS
// ============================================================================

uint32_t PageCacheManager::tick() {
  logical_clock++;
  return logical_clock;
}

int8_t PageCacheManager::findFreeLine(uint8_t set) {
  for (uint8_t w = 0; w < CACHE_NUM_WAYS; w++) {
    if (!cache_sets[set][w].valid) {
      return w;
    }
  }
  return -1;
}

int8_t PageCacheManager::findLRULine(uint8_t set) {
  uint32_t oldest = 0xFFFFFFFF;
  int8_t idx = 0;

  for (uint8_t w = 0; w < CACHE_NUM_WAYS; w++) {
    if (cache_sets[set][w].last_access < oldest) {
      oldest = cache_sets[set][w].last_access;
      idx = w;
    }
  }
  return idx;
}

void PageCacheManager::writeLineToSRAM(uint32_t addr, CacheLine *line) {
  sram.writeByteArray(addr, line->data, CACHE_LINE_SIZE);
}

void PageCacheManager::readLineFromSRAM(uint32_t addr, CacheLine *line) {
  sram.readByteArray(addr, line->data, CACHE_LINE_SIZE);
  line->tag = addr / CACHE_LINE_SIZE;
  line->valid = 1;
  line->dirty = 0;
  line->last_access = tick();
}

void PageCacheManager::touchPage(uint8_t pid, uint16_t logical_page) {
  PageTableEntry *pte = getPageEntry(pid, logical_page);
  if (pte && pte->valid) {
    pte->access_count++;
    pte->last_access = tick();
    if (pte->physical_page < MAX_PHYSICAL_PAGES) {
      phys_pages[pte->physical_page].access_time = tick();
    }
  }
}

// ============================================================================
// GESTION DE PAGINAS FISICAS
// ============================================================================

int16_t PageCacheManager::findFreePhysicalPage() {
  for (uint16_t i = 0; i < MAX_PHYSICAL_PAGES; i++) {
    if (phys_pages[i].state == PAGE_FREE) {
      return i;
    }
  }
  return -1;
}

int16_t PageCacheManager::findVictimPage() {
  uint32_t oldest_time = 0xFFFFFFFF;
  int16_t victim = -1;

  for (uint16_t i = 0; i < MAX_PHYSICAL_PAGES; i++) {
    if (phys_pages[i].state != PAGE_FREE && phys_pages[i].ref_count == 0) {
      if (phys_pages[i].access_time < oldest_time) {
        oldest_time = phys_pages[i].access_time;
        victim = i;
      }
    }
  }
  return victim;
}

int16_t PageCacheManager::allocatePhysicalPage(uint8_t owner_pid) {
  int16_t page = findFreePhysicalPage();

  if (page >= 0) {
    phys_pages[page].state = PAGE_ALLOCATED;
    phys_pages[page].owner_pid = owner_pid;
    phys_pages[page].ref_count = 1;
    phys_pages[page].access_time = tick();
    return page;
  }

  page = findVictimPage();

  if (page < 0) {
#ifdef BM_DEBUG
    Serial.println(F("[PageCache] ERROR: Sin paginas disponibles"));
#endif
    return -1;
  }

#ifdef BM_DEBUG
  Serial.print(F("[PageCache] Evicting PP="));
  Serial.print(page);
  Serial.print(F(" (owner="));
  Serial.print(phys_pages[page].owner_pid);
  Serial.println(F(")"));
#endif

  uint32_t page_addr = (uint32_t)page * PAGE_SIZE;
  for (uint8_t l = 0; l < LINES_PER_PAGE; l++) {
    invalidateLine(page_addr + l * CACHE_LINE_SIZE);
  }

  for (uint8_t p = 0; p < MAX_PROCESSES; p++) {
    for (uint16_t lp = 0; lp < MAX_PAGES_PER_PROC; lp++) {
      if (page_table[p][lp].valid && page_table[p][lp].physical_page == page) {
        page_table[p][lp].valid = 0;
        page_table[p][lp].dirty = 0;
        page_table[p][lp].access_count = 0;
      }
    }
  }

  phys_pages[page].owner_pid = owner_pid;
  phys_pages[page].ref_count = 1;
  phys_pages[page].access_time = tick();

  return page;
}

void PageCacheManager::freePhysicalPage(uint16_t phys_page) {
  if (phys_page >= MAX_PHYSICAL_PAGES) {
    return;
  }

  if (phys_pages[phys_page].ref_count > 0) {
    phys_pages[phys_page].ref_count--;
  }

  if (phys_pages[phys_page].ref_count == 0) {
    phys_pages[phys_page].state = PAGE_FREE;
    phys_pages[phys_page].owner_pid = 0;
    phys_pages[phys_page].access_time = 0;
  }
}

const PhysicalPageDescriptor *PageCacheManager::getPageInfo(uint16_t phys_page) const {
  if (phys_page >= MAX_PHYSICAL_PAGES) {
    return NULL;
  }
  return &phys_pages[phys_page];
}

// ============================================================================
// TABLA DE PAGINAS (MMU Software)
// ============================================================================

bool PageCacheManager::mapPage(uint8_t pid, uint16_t logical_page, uint16_t physical_page) {
  if (pid >= MAX_PROCESSES || logical_page >= MAX_PAGES_PER_PROC) {
    return false;
  }
  if (physical_page >= MAX_PHYSICAL_PAGES) {
    return false;
  }

  PageTableEntry *entry = &page_table[pid][logical_page];

  if (entry->valid) {
#ifdef BM_DEBUG
    Serial.print(F("[MMU] Remapeando PID="));
    Serial.print(pid);
    Serial.print(F(" LP="));
    Serial.print(logical_page);
    Serial.print(F(" PP="));
    Serial.print(entry->physical_page);
    Serial.print(F(" -> PP="));
    Serial.println(physical_page);
#endif
    freePhysicalPage(entry->physical_page);
  }

  entry->physical_page = physical_page;
  entry->valid = 1;
  entry->dirty = 0;
  entry->access_count = 0;
  entry->last_access = tick();

  phys_pages[physical_page].state = PAGE_MAPPED;
  phys_pages[physical_page].ref_count++;
  phys_pages[physical_page].access_time = tick();

  return true;
}

void PageCacheManager::unmapPage(uint8_t pid, uint16_t logical_page) {
  if (pid >= MAX_PROCESSES || logical_page >= MAX_PAGES_PER_PROC) {
    return;
  }

  PageTableEntry *entry = &page_table[pid][logical_page];
  if (!entry->valid) {
    return;
  }

  if (entry->dirty) {
    uint32_t phys_addr = (uint32_t)entry->physical_page * PAGE_SIZE;
    for (uint8_t l = 0; l < LINES_PER_PAGE; l++) {
      uint32_t line_addr = phys_addr + l * CACHE_LINE_SIZE;
      uint8_t set = (line_addr / CACHE_LINE_SIZE) % CACHE_NUM_SETS;
      uint32_t tag = line_addr / CACHE_LINE_SIZE;

      for (uint8_t w = 0; w < CACHE_NUM_WAYS; w++) {
        if (cache_sets[set][w].valid && cache_sets[set][w].tag == tag) {
          if (cache_sets[set][w].dirty) {
            writeLineToSRAM(line_addr, &cache_sets[set][w]);
            stats.writebacks++;
          }
          cache_sets[set][w].valid = 0;
          break;
        }
      }
    }
  }

  freePhysicalPage(entry->physical_page);

  entry->valid = 0;
  entry->dirty = 0;
  entry->access_count = 0;
}

uint32_t PageCacheManager::translateAddress(uint8_t pid, uint32_t logical_addr) const {
  if (pid >= MAX_PROCESSES) {
    return 0xFFFFFFFF;
  }

  uint16_t logical_page = logical_addr / PAGE_SIZE;
  uint16_t offset = logical_addr % PAGE_SIZE;

  if (logical_page >= MAX_PAGES_PER_PROC) {
    return 0xFFFFFFFF;
  }

  const PageTableEntry *entry = &page_table[pid][logical_page];
  if (!entry->valid) {
    return 0xFFFFFFFF;
  }

  return (uint32_t)entry->physical_page * PAGE_SIZE + offset;
}

PageTableEntry *PageCacheManager::getPageEntry(uint8_t pid, uint16_t logical_page) {
  if (pid >= MAX_PROCESSES || logical_page >= MAX_PAGES_PER_PROC) {
    return NULL;
  }
  return &page_table[pid][logical_page];
}

bool PageCacheManager::isValidAddress(uint8_t pid, uint32_t logical_addr) const {
  if (translateAddress(pid, logical_addr) != 0xFFFFFFFF) {
    return true;
  } else {
    return false;
  }
}

// ============================================================================
// OPERACIONES DE CACHE - LECTURA/ESCRITURA
// ============================================================================

uint8_t PageCacheManager::cacheReadByte(uint8_t pid, uint32_t logical_addr) {
  uint32_t phys_addr = translateAddress(pid, logical_addr);
  if (phys_addr == 0xFFFFFFFF) {
#ifdef BM_DEBUG
    Serial.print(F("[Cache] Read fault: PID="));
    Serial.print(pid);
    Serial.print(F(" addr=0x"));
    Serial.println(logical_addr, HEX);
#endif
    return 0;
  }

  uint32_t line_start = phys_addr - (phys_addr % CACHE_LINE_SIZE);
  uint16_t offset = phys_addr % CACHE_LINE_SIZE;
  uint8_t set = (phys_addr / CACHE_LINE_SIZE) % CACHE_NUM_SETS;
  uint32_t tag = phys_addr / CACHE_LINE_SIZE;

  for (uint8_t w = 0; w < CACHE_NUM_WAYS; w++) {
    if (cache_sets[set][w].valid && cache_sets[set][w].tag == tag) {
      cache_sets[set][w].last_access = tick();
      stats.hits++;
      stats.read_hits++;
      touchPage(pid, logical_addr / PAGE_SIZE);
      return cache_sets[set][w].data[offset];
    }
  }

  stats.misses++;
  stats.read_misses++;

  int8_t way = findFreeLine(set);
  if (way < 0) {
    way = findLRULine(set);
    if (cache_sets[set][way].dirty) {
      writeLineToSRAM(cache_sets[set][way].tag * CACHE_LINE_SIZE, &cache_sets[set][way]);
      stats.writebacks++;
    }
    stats.evictions++;
  }

  readLineFromSRAM(line_start, &cache_sets[set][way]);
  touchPage(pid, logical_addr / PAGE_SIZE);

  return cache_sets[set][way].data[offset];
}

void PageCacheManager::cacheWriteByte(uint8_t pid, uint32_t logical_addr, uint8_t data) {
  uint32_t phys_addr = translateAddress(pid, logical_addr);
  if (phys_addr == 0xFFFFFFFF) {
#ifdef BM_DEBUG
    Serial.print(F("[Cache] Write fault: PID="));
    Serial.print(pid);
    Serial.print(F(" addr=0x"));
    Serial.println(logical_addr, HEX);
#endif
    return;
  }

  uint32_t line_start = phys_addr - (phys_addr % CACHE_LINE_SIZE);
  uint16_t offset = phys_addr % CACHE_LINE_SIZE;
  uint8_t set = (phys_addr / CACHE_LINE_SIZE) % CACHE_NUM_SETS;
  uint32_t tag = phys_addr / CACHE_LINE_SIZE;

  for (uint8_t w = 0; w < CACHE_NUM_WAYS; w++) {
    if (cache_sets[set][w].valid && cache_sets[set][w].tag == tag) {
      cache_sets[set][w].data[offset] = data;
      cache_sets[set][w].dirty = 1;
      cache_sets[set][w].last_access = tick();
      stats.hits++;
      stats.write_hits++;

      sram.writeByte(phys_addr, data);

      PageTableEntry *pte = getPageEntry(pid, logical_addr / PAGE_SIZE);
      if (pte) {
        pte->dirty = 1;
      }

      touchPage(pid, logical_addr / PAGE_SIZE);
      return;
    }
  }

  stats.misses++;
  stats.write_misses++;

  int8_t way = findFreeLine(set);
  if (way < 0) {
    way = findLRULine(set);
    if (cache_sets[set][way].dirty) {
      writeLineToSRAM(cache_sets[set][way].tag * CACHE_LINE_SIZE, &cache_sets[set][way]);
      stats.writebacks++;
    }
    stats.evictions++;
  }

  readLineFromSRAM(line_start, &cache_sets[set][way]);
  cache_sets[set][way].data[offset] = data;
  cache_sets[set][way].dirty = 1;

  sram.writeByte(phys_addr, data);

  PageTableEntry *pte = getPageEntry(pid, logical_addr / PAGE_SIZE);
  if (pte) {
    pte->dirty = 1;
  }

  touchPage(pid, logical_addr / PAGE_SIZE);
}

void PageCacheManager::cacheReadArray(uint8_t pid, uint32_t logical_addr, uint8_t *buffer, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    buffer[i] = cacheReadByte(pid, logical_addr + i);
  }
}

void PageCacheManager::cacheWriteArray(uint8_t pid, uint32_t logical_addr, const uint8_t *data, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    cacheWriteByte(pid, logical_addr + i, data[i]);
  }
}

// ============================================================================
// TIPOS ESCALARES CON CACHE
// ============================================================================

void PageCacheManager::cacheWriteInt(uint8_t pid, uint32_t addr, int16_t data) {
  cacheWriteByte(pid, addr, (uint8_t)(data >> 8));
  cacheWriteByte(pid, addr + 1, (uint8_t)data);
}

int16_t PageCacheManager::cacheReadInt(uint8_t pid, uint32_t addr) {
  uint8_t h = cacheReadByte(pid, addr);
  uint8_t l = cacheReadByte(pid, addr + 1);
  return (int16_t)((h << 8) | l);
}

void PageCacheManager::cacheWriteUInt(uint8_t pid, uint32_t addr, uint16_t data) {
  cacheWriteByte(pid, addr, (uint8_t)(data >> 8));
  cacheWriteByte(pid, addr + 1, (uint8_t)data);
}

uint16_t PageCacheManager::cacheReadUInt(uint8_t pid, uint32_t addr) {
  uint8_t h = cacheReadByte(pid, addr);
  uint8_t l = cacheReadByte(pid, addr + 1);
  return (uint16_t)((h << 8) | l);
}

void PageCacheManager::cacheWriteLong(uint8_t pid, uint32_t addr, int32_t data) {
  cacheWriteByte(pid, addr, (uint8_t)(data >> 24));
  cacheWriteByte(pid, addr + 1, (uint8_t)(data >> 16));
  cacheWriteByte(pid, addr + 2, (uint8_t)(data >> 8));
  cacheWriteByte(pid, addr + 3, (uint8_t)data);
}

int32_t PageCacheManager::cacheReadLong(uint8_t pid, uint32_t addr) {
  uint32_t r = (uint32_t)cacheReadByte(pid, addr) << 24;
  r |= (uint32_t)cacheReadByte(pid, addr + 1) << 16;
  r |= (uint32_t)cacheReadByte(pid, addr + 2) << 8;
  r |= (uint32_t)cacheReadByte(pid, addr + 3);
  return (int32_t)r;
}

void PageCacheManager::cacheWriteULong(uint8_t pid, uint32_t addr, uint32_t data) {
  cacheWriteByte(pid, addr, (uint8_t)(data >> 24));
  cacheWriteByte(pid, addr + 1, (uint8_t)(data >> 16));
  cacheWriteByte(pid, addr + 2, (uint8_t)(data >> 8));
  cacheWriteByte(pid, addr + 3, (uint8_t)data);
}

uint32_t PageCacheManager::cacheReadULong(uint8_t pid, uint32_t addr) {
  uint32_t r = (uint32_t)cacheReadByte(pid, addr) << 24;
  r |= (uint32_t)cacheReadByte(pid, addr + 1) << 16;
  r |= (uint32_t)cacheReadByte(pid, addr + 2) << 8;
  r |= (uint32_t)cacheReadByte(pid, addr + 3);
  return r;
}

void PageCacheManager::cacheWriteFloat(uint8_t pid, uint32_t addr, float data) {
  uint8_t *p = (uint8_t *)&data;
  for (uint8_t i = 0; i < 4; i++) {
    cacheWriteByte(pid, addr + i, p[i]);
  }
}

float PageCacheManager::cacheReadFloat(uint8_t pid, uint32_t addr) {
  uint8_t b[4];
  for (uint8_t i = 0; i < 4; i++) {
    b[i] = cacheReadByte(pid, addr + i);
  }
  float f;
  memcpy(&f, b, 4);
  return f;
}

// ============================================================================
// GESTION DE CACHE
// ============================================================================

void PageCacheManager::invalidateLine(uint32_t physical_addr) {
  uint8_t set = (physical_addr / CACHE_LINE_SIZE) % CACHE_NUM_SETS;
  uint32_t tag = physical_addr / CACHE_LINE_SIZE;

  for (uint8_t w = 0; w < CACHE_NUM_WAYS; w++) {
    if (cache_sets[set][w].valid && cache_sets[set][w].tag == tag) {
      if (cache_sets[set][w].dirty) {
        writeLineToSRAM(physical_addr, &cache_sets[set][w]);
        stats.writebacks++;
      }
      cache_sets[set][w].valid = 0;
      return;
    }
  }
}

void PageCacheManager::invalidateProcessCache(uint8_t pid) {
  if (pid >= MAX_PROCESSES) {
    return;
  }

  for (uint16_t lp = 0; lp < MAX_PAGES_PER_PROC; lp++) {
    if (page_table[pid][lp].valid) {
      uint32_t page_addr = (uint32_t)page_table[pid][lp].physical_page * PAGE_SIZE;
      for (uint8_t l = 0; l < LINES_PER_PAGE; l++) {
        invalidateLine(page_addr + l * CACHE_LINE_SIZE);
      }
    }
  }
}

void PageCacheManager::flushCache() {
  for (uint8_t s = 0; s < CACHE_NUM_SETS; s++) {
    for (uint8_t w = 0; w < CACHE_NUM_WAYS; w++) {
      if (cache_sets[s][w].valid && cache_sets[s][w].dirty) {
        writeLineToSRAM(cache_sets[s][w].tag * CACHE_LINE_SIZE, &cache_sets[s][w]);
        cache_sets[s][w].dirty = 0;
        stats.writebacks++;
      }
      cache_sets[s][w].valid = 0;
    }
  }
}

void PageCacheManager::flushProcessCache(uint8_t pid) {
  if (pid >= MAX_PROCESSES) {
    return;
  }

  for (uint16_t lp = 0; lp < MAX_PAGES_PER_PROC; lp++) {
    if (page_table[pid][lp].valid) {
      uint32_t page_addr = (uint32_t)page_table[pid][lp].physical_page * PAGE_SIZE;
      for (uint8_t l = 0; l < LINES_PER_PAGE; l++) {
        uint32_t line_addr = page_addr + l * CACHE_LINE_SIZE;
        uint8_t set = (line_addr / CACHE_LINE_SIZE) % CACHE_NUM_SETS;
        uint32_t tag = line_addr / CACHE_LINE_SIZE;

        for (uint8_t w = 0; w < CACHE_NUM_WAYS; w++) {
          if (cache_sets[set][w].valid && cache_sets[set][w].tag == tag) {
            if (cache_sets[set][w].dirty) {
              writeLineToSRAM(line_addr, &cache_sets[set][w]);
              cache_sets[set][w].dirty = 0;
              stats.writebacks++;
            }
            break;
          }
        }
      }
      page_table[pid][lp].dirty = 0;
    }
  }
}

void PageCacheManager::invalidateAll() {
  for (uint8_t s = 0; s < CACHE_NUM_SETS; s++) {
    for (uint8_t w = 0; w < CACHE_NUM_WAYS; w++) {
      cache_sets[s][w].valid = 0;
      cache_sets[s][w].dirty = 0;
    }
  }
}

// ============================================================================
// ALLOCATOR CON PAGINACION
// ============================================================================

uint32_t PageCacheManager::pagedMalloc(uint8_t pid, uint16_t size) {
  if (pid >= MAX_PROCESSES || size == 0) {
    return 0;
  }

  uint16_t pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  if (pages_needed > MAX_PAGES_PER_PROC) {
#ifdef BM_DEBUG
    Serial.println(F("[PagedAlloc] Size excede maximo por proceso"));
#endif
    return 0;
  }

  uint16_t start_page = MAX_PAGES_PER_PROC;
  uint16_t contiguous = 0;

  for (uint16_t i = 0; i < MAX_PAGES_PER_PROC; i++) {
    if (!page_table[pid][i].valid) {
      if (contiguous == 0) {
        start_page = i;
      }
      contiguous++;
      if (contiguous >= pages_needed) {
        break;
      }
    } else {
      contiguous = 0;
    }
  }

  if (contiguous < pages_needed) {
#ifdef BM_DEBUG
    Serial.print(F("[PagedAlloc] Sin espacio contiguo para "));
    Serial.print(pages_needed);
    Serial.println(F(" paginas"));
#endif
    return 0;
  }

  for (uint16_t i = 0; i < pages_needed; i++) {
    int16_t phys = allocatePhysicalPage(pid);
    if (phys < 0) {
      for (uint16_t j = 0; j < i; j++) {
        unmapPage(pid, start_page + j);
      }
#ifdef BM_DEBUG
      Serial.println(F("[PagedAlloc] No hay paginas fisicas disponibles"));
#endif
      return 0;
    }

    if (!mapPage(pid, start_page + i, (uint16_t)phys)) {
      freePhysicalPage((uint16_t)phys);
      for (uint16_t j = 0; j < i; j++) {
        unmapPage(pid, start_page + j);
      }
      return 0;
    }
  }

  uint32_t result = (uint32_t)start_page * PAGE_SIZE;

#ifdef BM_DEBUG
  Serial.print(F("[PagedAlloc] PID="));
  Serial.print(pid);
  Serial.print(F(" size="));
  Serial.print(size);
  Serial.print(F(" pages="));
  Serial.print(pages_needed);
  Serial.print(F(" logical_addr=0x"));
  Serial.println(result, HEX);
#endif

  return result;
}

void PageCacheManager::pagedFree(uint8_t pid, uint32_t logical_addr) {
  if (pid >= MAX_PROCESSES || logical_addr == 0) {
    return;
  }

  uint16_t start_page = logical_addr / PAGE_SIZE;
  if (start_page >= MAX_PAGES_PER_PROC) {
    return;
  }

  if (!page_table[pid][start_page].valid) {
    return;
  }

  uint16_t count = 0;
  for (uint16_t i = start_page; i < MAX_PAGES_PER_PROC; i++) {
    if (page_table[pid][i].valid) {
      count++;
    } else {
      break;
    }
  }

#ifdef BM_DEBUG
  Serial.print(F("[PagedFree] PID="));
  Serial.print(pid);
  Serial.print(F(" addr=0x"));
  Serial.print(logical_addr, HEX);
  Serial.print(F(" pages="));
  Serial.println(count);
#endif

  for (uint16_t i = 0; i < count; i++) {
    unmapPage(pid, start_page + i);
  }
}

uint32_t PageCacheManager::pagedRealloc(uint8_t pid, uint32_t old_addr, uint16_t new_size) {
  if (old_addr == 0) {
    return pagedMalloc(pid, new_size);
  }
  if (new_size == 0) {
    pagedFree(pid, old_addr);
    return 0;
  }

  uint16_t old_start = old_addr / PAGE_SIZE;
  if (old_start >= MAX_PAGES_PER_PROC || !page_table[pid][old_start].valid) {
    return 0;
  }

  uint16_t old_pages = 0;
  for (uint16_t i = old_start; i < MAX_PAGES_PER_PROC; i++) {
    if (page_table[pid][i].valid) {
      old_pages++;
    } else {
      break;
    }
  }
  uint16_t old_size = old_pages * PAGE_SIZE;

  if (new_size <= old_size) {
    return old_addr;
  }

  uint32_t new_addr = pagedMalloc(pid, new_size);
  if (new_addr == 0) {
    return 0;
  }

  uint16_t copy_len = (old_size < new_size) ? old_size : new_size;
  for (uint16_t i = 0; i < copy_len; i++) {
    uint8_t b = cacheReadByte(pid, old_addr + i);
    cacheWriteByte(pid, new_addr + i, b);
  }

  pagedFree(pid, old_addr);
  return new_addr;
}

// ============================================================================
// CONSULTAS Y ESTADISTICAS
// ============================================================================

CacheStats PageCacheManager::getStats() const {
  return stats;
}

void PageCacheManager::resetStats() {
  memset(&stats, 0, sizeof(stats));
}

uint16_t PageCacheManager::getFreePages() const {
  uint16_t count = 0;
  for (uint16_t i = 0; i < MAX_PHYSICAL_PAGES; i++) {
    if (phys_pages[i].state == PAGE_FREE) {
      count++;
    }
  }
  return count;
}

uint16_t PageCacheManager::getUsedPages() const {
  return MAX_PHYSICAL_PAGES - getFreePages();
}

uint16_t PageCacheManager::getProcessPages(uint8_t pid) const {
  if (pid >= MAX_PROCESSES) {
    return 0;
  }

  uint16_t count = 0;
  for (uint16_t i = 0; i < MAX_PAGES_PER_PROC; i++) {
    if (page_table[pid][i].valid) {
      count++;
    }
  }
  return count;
}

uint32_t PageCacheManager::getProcessMemory(uint8_t pid) const {
  return (uint32_t)getProcessPages(pid) * PAGE_SIZE;
}

// ============================================================================
// DEBUG
// ============================================================================

void PageCacheManager::printCacheState() {
#ifdef BM_DEBUG
  Serial.println(F("╔══════════════════════════════════════╗"));
  Serial.println(F("║          CACHE L1 STATE              ║"));
  Serial.println(F("╠══════════════════════════════════════╣"));

  Serial.print(F("║ Hits: "));
  Serial.print(stats.hits);
  Serial.print(F("  Misses: "));
  Serial.print(stats.misses);
  Serial.print(F("  Rate: "));
  Serial.print(stats.getHitRate());
  Serial.println(F("%      ║"));

  Serial.print(F("║ R-Hit: "));
  Serial.print(stats.read_hits);
  Serial.print(F(" R-Miss: "));
  Serial.print(stats.read_misses);
  Serial.print(F(" W-Hit: "));
  Serial.print(stats.write_hits);
  Serial.print(F(" W-Miss: "));
  Serial.println(stats.write_misses);

  Serial.print(F("║ Evictions: "));
  Serial.print(stats.evictions);
  Serial.print(F("  Writebacks: "));
  Serial.println(stats.writebacks);

  Serial.println(F("╠══════════════════════════════════════╣"));
  Serial.println(F("║ Cache Lines:                         ║"));

  for (uint8_t s = 0; s < CACHE_NUM_SETS; s++) {
    Serial.print(F("║ Set "));
    Serial.print(s);
    Serial.print(F(": "));
    for (uint8_t w = 0; w < CACHE_NUM_WAYS; w++) {
      CacheLine *cl = &cache_sets[s][w];
      if (cl->valid) {
        Serial.print(F("[V"));
        if (cl->dirty) {
          Serial.print(F("D"));
        } else {
          Serial.print(F(" "));
        }
        Serial.print(F(":0x"));
        if (cl->tag < 0x10) {
          Serial.print('0');
        }
        Serial.print(cl->tag, HEX);
        Serial.print(F("] "));
      } else {
        Serial.print(F("[  empty  ] "));
      }
    }
    Serial.println(F("║"));
  }

  Serial.println(F("╚══════════════════════════════════════╝"));
#endif
}

void PageCacheManager::printPageTable(uint8_t pid) {
#ifdef BM_DEBUG
  if (pid >= MAX_PROCESSES) {
    return;
  }

  Serial.print(F("╔══════════════════════════════════════╗\n"));
  Serial.print(F("║ Page Table PID="));
  if (pid < 10) {
    Serial.print(' ');
  }
  Serial.print(pid);
  Serial.println(F("                   ║"));
  Serial.println(F("╠══════════════════════════════════════╣"));
  Serial.println(F("║ LP   → PP   State     Access      ║"));
  Serial.println(F("╠══════════════════════════════════════╣"));

  bool has_pages = false;
  for (uint16_t i = 0; i < MAX_PAGES_PER_PROC; i++) {
    PageTableEntry *e = &page_table[pid][i];
    if (e->valid) {
      has_pages = true;
      Serial.print(F("║ "));
      if (i < 10) {
        Serial.print(' ');
      }
      Serial.print(i);
      Serial.print(F("  → "));
      if (e->physical_page < 100) {
        Serial.print(' ');
      }
      if (e->physical_page < 10) {
        Serial.print(' ');
      }
      Serial.print(e->physical_page);
      Serial.print(F("  "));
      if (e->dirty) {
        Serial.print(F("DIRTY   "));
      } else {
        Serial.print(F("CLEAN   "));
      }
      Serial.print(e->access_count);
      Serial.println(F("          ║"));
    }
  }

  if (!has_pages) {
    Serial.println(F("║         (no pages mapped)          ║"));
  }

  Serial.println(F("╚══════════════════════════════════════╝"));
#endif
}

void PageCacheManager::printPhysicalPageMap() {
#ifdef BM_DEBUG
  uint16_t free_c = 0;
  uint16_t alloc_c = 0;
  uint16_t mapped_c = 0;

  for (uint16_t i = 0; i < MAX_PHYSICAL_PAGES; i++) {
    switch (phys_pages[i].state) {
      case PAGE_FREE:
        {
          free_c++;
          break;
        }
      case PAGE_ALLOCATED:
        {
          alloc_c++;
          break;
        }
      case PAGE_MAPPED:
        {
          mapped_c++;
          break;
        }
    }
  }

  Serial.println(F("╔══════════════════════════════════════╗"));
  Serial.println(F("║      PHYSICAL PAGE MAP              ║"));
  Serial.println(F("╠══════════════════════════════════════╣"));
  Serial.print(F("║ Free: "));
  Serial.print(free_c);
  Serial.print(F("  Alloc: "));
  Serial.print(alloc_c);
  Serial.print(F("  Mapped: "));
  Serial.println(mapped_c);
  Serial.println(F("╠══════════════════════════════════════╣"));

  uint8_t shown = 0;
  for (uint16_t i = 0; i < MAX_PHYSICAL_PAGES; i++) {
    if (shown >= 10) {
      break;
    }
    if (phys_pages[i].state != PAGE_FREE) {
      Serial.print(F("║ PP="));
      if (i < 100) {
        Serial.print(' ');
      }
      if (i < 10) {
        Serial.print(' ');
      }
      Serial.print(i);
      Serial.print(F(" owner="));
      Serial.print(phys_pages[i].owner_pid);
      Serial.print(F(" ref="));
      Serial.println(phys_pages[i].ref_count);
      shown++;
    }
  }

  uint16_t total_used = alloc_c + mapped_c;
  if (total_used > 10) {
    Serial.print(F("║ ... and "));
    Serial.print(total_used - 10);
    Serial.println(F(" more                ║"));
  }

  Serial.println(F("╚══════════════════════════════════════╝"));
#endif
}

void PageCacheManager::printSummary() {
#ifdef BM_DEBUG
  Serial.println(F("\n┌──────────────────────────────────────┐"));
  Serial.println(F("│    PAGECACHE SYSTEM SUMMARY         │"));
  Serial.println(F("├──────────────────────────────────────┤"));
  Serial.print(F("│ Paginas libres:  "));
  Serial.print(getFreePages());
  Serial.println(F("              │"));
  Serial.print(F("│ Paginas usadas:  "));
  Serial.print(getUsedPages());
  Serial.println(F("              │"));
  Serial.print(F("│ Cache hit rate: "));
  Serial.print(stats.getHitRate());
  Serial.println(F("%             │"));
  Serial.print(F("│ Cache evictions: "));
  Serial.print(stats.evictions);
  Serial.println(F("             │"));
  Serial.println(F("└──────────────────────────────────────┘\n"));
#endif
}

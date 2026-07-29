/*
 * Conexiones 23LC1024:
 *   VCC  -> 5V
 *   GND  -> GND  
 *   CS   -> D10
 *   SO   -> D12 (MISO)
 *   SI   -> D11 (MOSI)
 *   SCK  -> D13
 *   HOLD -> VCC
 */

/*
 * Arduino_RAM_Dinamica_V3b.ino
 *
 * Ejemplo completo del sistema de paginación y caché
 */

#include "BareMetalRAM.h"
#include <string.h>
#include <stdio.h>

#define BM_DEBUG

// ============================================================================
// ALMACENAMIENTO DE DIRECCIONES POR PROCESO
// ============================================================================

struct ProcMemInfo {
  uint32_t sensor_addr;
  uint32_t data_addr;
  uint16_t sensor_size;
  uint16_t data_size;
  bool initialized;
};

ProcMemInfo proc_mem[MAX_PROCESSES];

// ============================================================================
// TAREA 1: Sensor con paginación
// ============================================================================

void tarea_sensor_paginado(Process *p) {
  ProcMemInfo *info = &proc_mem[p->pid % MAX_PROCESSES];

  if (!info->initialized) {
    info->sensor_size = 16;
    info->sensor_addr = PageCache.pagedMalloc(p->pid, info->sensor_size);

    if (info->sensor_addr == 0) {
      Serial.print(F("[Sensor] PID="));
      Serial.print(p->pid);
      Serial.println(F(" ERROR: pagedMalloc falló"));
      return;
    }

    info->initialized = true;
    Serial.print(F("[Sensor] PID="));
    Serial.print(p->pid);
    Serial.print(F(" Alloc OK: 0x"));
    Serial.println(info->sensor_addr, HEX);
  }

  float temp = 25.0 + random(-50, 50) / 10.0;
  float hum = 60.0 + random(-100, 100) / 10.0;

  PageCache.cacheWriteFloat(p->pid, info->sensor_addr, temp);
  PageCache.cacheWriteFloat(p->pid, info->sensor_addr + 4, hum);

  float temp_read = PageCache.cacheReadFloat(p->pid, info->sensor_addr);
  float hum_read = PageCache.cacheReadFloat(p->pid, info->sensor_addr + 4);

  Serial.print(F("[Sensor] PID="));
  Serial.print(p->pid);
  Serial.print(F(" T="));
  Serial.print(temp_read, 1);
  Serial.print(F("°C H="));
  Serial.print(hum_read, 1);
  Serial.println(F("%"));
}

// ============================================================================
// TAREA 2: Logger con paginación
// ============================================================================

void tarea_logger_paginado(Process *p) {
  ProcMemInfo *info = &proc_mem[p->pid % MAX_PROCESSES];

  if (!info->initialized) {
    info->data_size = 64;
    info->data_addr = PageCache.pagedMalloc(p->pid, info->data_size);

    if (info->data_addr == 0) {
      Serial.print(F("[Logger] PID="));
      Serial.print(p->pid);
      Serial.println(F(" ERROR: pagedMalloc falló"));
      return;
    }

    info->initialized = true;
    Serial.print(F("[Logger] PID="));
    Serial.print(p->pid);
    Serial.print(F(" Alloc OK: 0x"));
    Serial.println(info->data_addr, HEX);
  }

  char msg[32];
  snprintf(msg, sizeof(msg), "LOG t=%lu", millis());

  for (uint8_t i = 0; msg[i] != '\0'; i++) {
    PageCache.cacheWriteByte(p->pid, info->data_addr + i, msg[i]);
  }
  PageCache.cacheWriteByte(p->pid, info->data_addr + strlen(msg), 0);

  char read_msg[32];
  for (uint8_t i = 0; i < 31; i++) {
    read_msg[i] = PageCache.cacheReadByte(p->pid, info->data_addr + i);
    if (read_msg[i] == 0) {
      break;
    }
  }
  read_msg[31] = 0;

  Serial.print(F("[Logger] PID="));
  Serial.print(p->pid);
  Serial.print(F(" "));
  Serial.println(read_msg);
}

// ============================================================================
// TAREA 3: Stress test de caché
// ============================================================================

void tarea_cache_stress(Process *p) {
  ProcMemInfo *info = &proc_mem[p->pid % MAX_PROCESSES];

  if (!info->initialized) {
    info->data_size = PAGE_SIZE;
    info->data_addr = PageCache.pagedMalloc(p->pid, PAGE_SIZE);

    if (info->data_addr == 0) {
      return;
    }

    for (uint16_t i = 0; i < PAGE_SIZE; i++) {
      PageCache.cacheWriteByte(p->pid, info->data_addr + i, i & 0xFF);
    }

    info->initialized = true;
  }

  static uint16_t offset = 0;

  Serial.print(F("[Stress] PID="));
  Serial.print(p->pid);
  Serial.print(F(" @"));
  Serial.print(offset);
  Serial.print(F(": "));

  for (uint8_t i = 0; i < 8; i++) {
    uint8_t val = PageCache.cacheReadByte(p->pid, info->data_addr + offset);
    Serial.print(val, HEX);
    Serial.print(F(" "));
    offset = (offset + 1) % PAGE_SIZE;
  }
  Serial.println();
}

// ============================================================================
// CLEANUP: Liberar memoria paginada
// ============================================================================

void cleanup_paginado(Process *p) {
  ProcMemInfo *info = &proc_mem[p->pid % MAX_PROCESSES];

  Serial.print(F("[Cleanup] PID="));
  Serial.print(p->pid);

  if (info->sensor_addr != 0) {
    PageCache.pagedFree(p->pid, info->sensor_addr);
    Serial.print(F(" sensor freed"));
  }
  if (info->data_addr != 0) {
    PageCache.pagedFree(p->pid, info->data_addr);
    Serial.print(F(" data freed"));
  }

  info->sensor_addr = 0;
  info->data_addr = 0;
  info->initialized = false;

  Serial.println();
}

// ============================================================================
// TAREA TEMPORIZADA PARA ESTADÍSTICAS (cada 8 segundos)
// ============================================================================

void stats_task() {
  Serial.println();
  PageCache.printSummary();
  PageCache.printCacheState();

  for (uint8_t i = 0; i < MAX_PROCESSES; i++) {
    Process *p = PM.getByIndex(i);
    if (p) {
      PageCache.printPageTable(p->pid);
    }
  }
}

// ============================================================================
// SETUP Y LOOP
// ============================================================================

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  Serial.println(F("\n╔══════════════════════════════════════════════╗"));
  Serial.println(F("║  BareMetalRAM - Paginación y Caché L1       ║"));
  Serial.println(F("╚══════════════════════════════════════════════╝\n"));

  BMRAM.begin(10);
  memset(proc_mem, 0, sizeof(proc_mem));

  PM.create("sensor", tarea_sensor_paginado, PROC_USER, PRIO_HIGH, 0, 1000000, true);    // 1s
  PM.create("logger", tarea_logger_paginado, PROC_USER, PRIO_NORMAL, 0, 2000000, true);  // 2s
  PM.create("stress", tarea_cache_stress, PROC_USER, PRIO_LOW, 0, 500000, true);         // 0.5s

  // Registrar tarea periódica de estadísticas (8 segundos)
  static TimedTask statsTask(8000000UL, stats_task);  // intervalo en microsegundos
  TaskScheduler::registerTask(&statsTask);

  Serial.print(F("\nProcesos activos: "));
  Serial.println(PM.count());
  Serial.print(F("Páginas libres: "));
  Serial.println(PageCache.getFreePages());
  Serial.print(F("Heap clásico libre: "));
  Serial.println(ExtHeap.getFree());  // CAMBIADO: ExtSRAM -> ExtHeap

  Serial.println(F("\nIniciando scheduler...\n"));
}

void loop() {
  PM.schedule();
  TaskScheduler::checkAll();  // Ejecuta tareas temporizadas (incluye stats_task)
}

// ┌──────────────────────────────────────────────────────────────┐
// │                    PROCESO (PID)                             │
// │  ┌───────────────────────────────────────────────────────┐   │
// │  │         ESPACIO DE DIRECCIONES LÓGICAS                │   │
// │  │  0x0000 ┌──────────┐     0x0100 ┌──────────┐          │   │
// │  │         │ Pág L0   │            │ Pág L1   │  ...     │   │
// │  │         └────┬─────┘            └────┬─────┘          │   │
// │  └──────────────┼───────────────────────┼────────────────┘   │
// │                 │    TABLA DE PÁGINAS   │                    │
// │                 ▼                       ▼                    │
// │  ┌──────────────┹───────────────────────┹─────────────────┐  │
// │  │              PÁGINAS FÍSICAS (SRAM 23LC1024)           │  │
// │  │  PP0  PP1  PP2  ...  PP509  PP510  PP511               │  │
// │  │  ┌───┐┌───┐┌───┐     ┌───┐  ┌───┐  ┌───┐               │  │
// │  │  │256││256││256│ ... │256│  │256│  │256│ bytes         │  │
// │  │  └───┘└───┘└───┘     └───┘  └───┘  └───┘               │  │
// │  └───────────────────────────┬────────────────────────────┘  │
// │                              │                               │
// │                              ▼                               │
// │  ┌───────────────────────────┹────────────────────────────┐  │
// │  │                    CACHÉ L1 (SRAM Interna)             │  │
// │  │     Set 0        Set 1        Set 2        Set 3       │  │
// │  │   ┌─┬─┬─┬─┐    ┌─┬─┬─┬─┐    ┌─┬─┬─┬─┐    ┌─┬─┬─┬─┐     │  │
// │  │   │W│W│W│W│    │W│W│W│W│    │W│W│W│W│    │W│W│W│W│     │  │
// │  │   │0│1│2│3│    │0│1│2│3│    │0│1│2│3│    │0│1│2│3│     │  │
// │  │   └─┴─┴─┴─┘    └─┴─┴─┴─┘    └─┴─┴─┴─┘    └─┴─┴─┴─┘     │  │
// │  │   32 bytes/way  → 512 bytes total                      │  │
// │  └────────────────────────────────────────────────────────┘  │
// └──────────────────────────────────────────────────────────────┘

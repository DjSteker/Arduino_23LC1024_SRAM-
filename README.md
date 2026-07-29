# Arduino 23LC1024 SRAM
Ejemplo del sistema de paginación con caché y procesos 


BareMetalRAM — Sistema de paginación con caché y gestión de procesos para AVR/ATmega328P

Ejemplo completo de un kernel cooperativo ligero para Arduino que implementa memoria virtual paginada sobre una SRAM externa SPI (23LC1024, 128 KB), con una caché L1 de 4 vías en la SRAM interna del microcontrolador.

Características principales:

Paginación por software (MMU virtual): páginas lógicas de 256 bytes mapeadas a páginas físicas mediante una tabla de páginas por proceso (PageTableEntry), con protección de memoria entre procesos por PID.
Caché L1 (4-way set associative, 512 B): política write-through con seguimiento de bit dirty, reemplazo LRU basado en un reloj lógico interno (sin depender de micros()), y writeback automático en evictions.
Gestión de procesos cooperativa: ProcessManager con tabla de procesos, prioridades, jerarquía padre/hijo con conteo de referencias, y ciclo de vida completo (READY → RUNNING → KILL_PENDING → ZOMBIE) recolectado por un garbage collector simple.

Scheduler temporizado: TimedTask/TaskScheduler basado en Timer1 en modo CTC (resolución de microsegundo), tolerante a overflow de 32 bits, para tareas periódicas independientes del scheduler de procesos.
Allocator paginado (pagedMalloc/pagedFree/pagedRealloc): asignación de memoria por proceso en páginas contiguas, con eviction automática de páginas físicas cuando no hay marcos libres.
Heap clásico auxiliar (ExtHeap): allocator first-fit adicional sobre la misma SRAM externa, para asignaciones que no requieren paginación.


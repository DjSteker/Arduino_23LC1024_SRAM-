/*
 * SRAMsimple.h
 *
 *  Created on: 24 jul 2026
 *      Author: DjSteker
 */

#ifndef SRAMSIMPLE_H
#define SRAMSIMPLE_H

#include <Arduino.h>
#include <SPI.h>

// Opcodes 23LC1024 / 23A256
#define RDMR 0x05
#define WRMR 0x01
#define READ_CMD 0x03
#define WRITE_CMD 0x02
#define RSTIO 0xFF
#define BYTE_MODE 0x00
#define SEQ_MODE 0x40

class SRAMsimple {
public:
  SRAMsimple();
  void begin(byte csPin = 10, byte mode = SEQ_MODE);

  // Byte
  void writeByte(uint32_t addr, byte data);
  byte readByte(uint32_t addr);

  // Array de bytes (modo secuencial)
  void writeByteArray(uint32_t addr, byte *data, uint16_t len);
  void readByteArray(uint32_t addr, byte *data, uint16_t len);

  // Tipos escalares (sin VLA en stack)
  void writeInt(uint32_t addr, int data);
  int readInt(uint32_t addr);
  void writeUInt(uint32_t addr, unsigned int data);
  unsigned int readUInt(uint32_t addr);
  void writeLong(uint32_t addr, long data);
  long readLong(uint32_t addr);
  void writeULong(uint32_t addr, unsigned long data);
  unsigned long readULong(uint32_t addr);
  void writeFloat(uint32_t addr, float data);
  float readFloat(uint32_t addr);

  // Arrays de tipos (descompuestos en loop para no usar stack grande)
  void writeIntArray(uint32_t addr, int *data, uint16_t n);
  void readIntArray(uint32_t addr, int *data, uint16_t n);
  void writeUIntArray(uint32_t addr, unsigned int *data, uint16_t n);
  void readUIntArray(uint32_t addr, unsigned int *data, uint16_t n);
  void writeLongArray(uint32_t addr, long *data, uint16_t n);
  void readLongArray(uint32_t addr, long *data, uint16_t n);
  void writeULongArray(uint32_t addr, unsigned long *data, uint16_t n);
  void readULongArray(uint32_t addr, unsigned long *data, uint16_t n);
  void writeFloatArray(uint32_t addr, float *data, uint16_t n);
  void readFloatArray(uint32_t addr, float *data, uint16_t n);

private:
  byte _cs;
  void setMode(byte mode);
  inline void select(void) {
    digitalWrite(_cs, LOW);
  }
  inline void deselect(void) {
    digitalWrite(_cs, HIGH);
  }
};

#endif /* SRAMSIMPLE_H */

/*
 * SRAMsimple.cpp
 *
 *  Created on: 24 jul 2026
 *      Author: DjSteker
 */

#include "SRAMsimple.h"

SRAMsimple::SRAMsimple()
  : _cs(10) {
}

void SRAMsimple::begin(byte csPin, byte mode) {
  _cs = csPin;
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);
  SPI.begin();
  setMode(mode);
}

void SRAMsimple::setMode(byte mode) {
  select();
  SPI.transfer(WRMR);
  SPI.transfer(mode);
  deselect();
}

// ---------- Byte ----------
void SRAMsimple::writeByte(uint32_t addr, byte data) {
  select();
  SPI.transfer(WRITE_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  SPI.transfer(data);
  deselect();
}

byte SRAMsimple::readByte(uint32_t addr) {
  byte b;
  select();
  SPI.transfer(READ_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  b = SPI.transfer(0x00);
  deselect();
  return b;
}

// ---------- Byte Array ----------
void SRAMsimple::writeByteArray(uint32_t addr, byte *data, uint16_t len) {
  select();
  SPI.transfer(WRITE_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  for (uint16_t i = 0; i < len; i++) {
    SPI.transfer(data[i]);
  }
  deselect();
}

void SRAMsimple::readByteArray(uint32_t addr, byte *data, uint16_t len) {
  select();
  SPI.transfer(READ_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  for (uint16_t i = 0; i < len; i++) {
    data[i] = SPI.transfer(0x00);
  }
  deselect();
}

// ---------- Int ----------
void SRAMsimple::writeInt(uint32_t addr, int data) {
  select();
  SPI.transfer(WRITE_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  SPI.transfer((byte)(data >> 8));
  SPI.transfer((byte)data);
  deselect();
}

int SRAMsimple::readInt(uint32_t addr) {
  byte h, l;
  select();
  SPI.transfer(READ_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  h = SPI.transfer(0x00);
  l = SPI.transfer(0x00);
  deselect();
  return ((int)h << 8) | l;
}

// ---------- Unsigned Int ----------
void SRAMsimple::writeUInt(uint32_t addr, unsigned int data) {
  select();
  SPI.transfer(WRITE_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  SPI.transfer((byte)(data >> 8));
  SPI.transfer((byte)data);
  deselect();
}

unsigned int SRAMsimple::readUInt(uint32_t addr) {
  byte h, l;
  select();
  SPI.transfer(READ_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  h = SPI.transfer(0x00);
  l = SPI.transfer(0x00);
  deselect();
  return ((unsigned int)h << 8) | l;
}

// ---------- Long ----------
void SRAMsimple::writeLong(uint32_t addr, long data) {
  select();
  SPI.transfer(WRITE_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  SPI.transfer((byte)(data >> 24));
  SPI.transfer((byte)(data >> 16));
  SPI.transfer((byte)(data >> 8));
  SPI.transfer((byte)data);
  deselect();
}

long SRAMsimple::readLong(uint32_t addr) {
  byte t[4];
  select();
  SPI.transfer(READ_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  for (uint8_t i = 0; i < 4; i++) {
    t[i] = SPI.transfer(0x00);
  }
  deselect();
  return ((long)t[0] << 24) | ((long)t[1] << 16) | ((long)t[2] << 8) | t[3];
}

// ---------- Unsigned Long ----------
void SRAMsimple::writeULong(uint32_t addr, unsigned long data) {
  select();
  SPI.transfer(WRITE_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  SPI.transfer((byte)(data >> 24));
  SPI.transfer((byte)(data >> 16));
  SPI.transfer((byte)(data >> 8));
  SPI.transfer((byte)data);
  deselect();
}

unsigned long SRAMsimple::readULong(uint32_t addr) {
  byte t[4];
  select();
  SPI.transfer(READ_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  for (uint8_t i = 0; i < 4; i++) {
    t[i] = SPI.transfer(0x00);
  }
  deselect();
  return ((unsigned long)t[0] << 24) | ((unsigned long)t[1] << 16) | ((unsigned long)t[2] << 8) | t[3];
}

// ---------- Float ----------
void SRAMsimple::writeFloat(uint32_t addr, float data) {
  byte *t = (byte *)&data;
  select();
  SPI.transfer(WRITE_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  SPI.transfer(t[0]);
  SPI.transfer(t[1]);
  SPI.transfer(t[2]);
  SPI.transfer(t[3]);
  deselect();
}

float SRAMsimple::readFloat(uint32_t addr) {
  byte t[4];
  select();
  SPI.transfer(READ_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  for (uint8_t i = 0; i < 4; i++) {
    t[i] = SPI.transfer(0x00);
  }
  deselect();
  return *(float *)t;
}

// ---------- Int Array ----------
void SRAMsimple::writeIntArray(uint32_t addr, int *data, uint16_t n) {
  select();
  SPI.transfer(WRITE_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  for (uint16_t i = 0; i < n; i++) {
    SPI.transfer((byte)(data[i] >> 8));
    SPI.transfer((byte)data[i]);
  }
  deselect();
}

void SRAMsimple::readIntArray(uint32_t addr, int *data, uint16_t n) {
  select();
  SPI.transfer(READ_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  for (uint16_t i = 0; i < n; i++) {
    byte h = SPI.transfer(0x00);
    byte l = SPI.transfer(0x00);
    data[i] = ((int)h << 8) | l;
  }
  deselect();
}

// ---------- UInt Array ----------
void SRAMsimple::writeUIntArray(uint32_t addr, unsigned int *data, uint16_t n) {
  select();
  SPI.transfer(WRITE_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  for (uint16_t i = 0; i < n; i++) {
    SPI.transfer((byte)(data[i] >> 8));
    SPI.transfer((byte)data[i]);
  }
  deselect();
}

void SRAMsimple::readUIntArray(uint32_t addr, unsigned int *data, uint16_t n) {
  select();
  SPI.transfer(READ_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  for (uint16_t i = 0; i < n; i++) {
    byte h = SPI.transfer(0x00);
    byte l = SPI.transfer(0x00);
    data[i] = ((unsigned int)h << 8) | l;
  }
  deselect();
}

// ---------- Long Array ----------
void SRAMsimple::writeLongArray(uint32_t addr, long *data, uint16_t n) {
  select();
  SPI.transfer(WRITE_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  for (uint16_t i = 0; i < n; i++) {
    SPI.transfer((byte)(data[i] >> 24));
    SPI.transfer((byte)(data[i] >> 16));
    SPI.transfer((byte)(data[i] >> 8));
    SPI.transfer((byte)data[i]);
  }
  deselect();
}

void SRAMsimple::readLongArray(uint32_t addr, long *data, uint16_t n) {
  select();
  SPI.transfer(READ_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  for (uint16_t i = 0; i < n; i++) {
    byte t[4];
    for (uint8_t j = 0; j < 4; j++) {
      t[j] = SPI.transfer(0x00);
    }
    data[i] = ((long)t[0] << 24) | ((long)t[1] << 16) | ((long)t[2] << 8) | t[3];
  }
  deselect();
}

// ---------- ULong Array ----------
void SRAMsimple::writeULongArray(uint32_t addr, unsigned long *data, uint16_t n) {
  select();
  SPI.transfer(WRITE_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  for (uint16_t i = 0; i < n; i++) {
    SPI.transfer((byte)(data[i] >> 24));
    SPI.transfer((byte)(data[i] >> 16));
    SPI.transfer((byte)(data[i] >> 8));
    SPI.transfer((byte)data[i]);
  }
  deselect();
}

void SRAMsimple::readULongArray(uint32_t addr, unsigned long *data, uint16_t n) {
  select();
  SPI.transfer(READ_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  for (uint16_t i = 0; i < n; i++) {
    byte t[4];
    for (uint8_t j = 0; j < 4; j++) {
      t[j] = SPI.transfer(0x00);
    }
    data[i] = ((unsigned long)t[0] << 24) | ((unsigned long)t[1] << 16) | ((unsigned long)t[2] << 8) | t[3];
  }
  deselect();
}

// ---------- Float Array ----------
void SRAMsimple::writeFloatArray(uint32_t addr, float *data, uint16_t n) {
  select();
  SPI.transfer(WRITE_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  for (uint16_t i = 0; i < n; i++) {
    byte *t = (byte *)&data[i];
    SPI.transfer(t[0]);
    SPI.transfer(t[1]);
    SPI.transfer(t[2]);
    SPI.transfer(t[3]);
  }
  deselect();
}

void SRAMsimple::readFloatArray(uint32_t addr, float *data, uint16_t n) {
  select();
  SPI.transfer(READ_CMD);
  SPI.transfer((byte)(addr >> 16));
  SPI.transfer((byte)(addr >> 8));
  SPI.transfer((byte)addr);
  for (uint16_t i = 0; i < n; i++) {
    byte t[4];
    for (uint8_t j = 0; j < 4; j++) {
      t[j] = SPI.transfer(0x00);
    }
    data[i] = *(float *)t;
  }
  deselect();
}

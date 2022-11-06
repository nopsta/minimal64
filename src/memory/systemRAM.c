#include "../m64.h"

#define SYSTEM_RAM_LENGTH 0x10000
uint8_t SYSTEMRAM[SYSTEM_RAM_LENGTH];

void systemram_reset() {
  int i, j;

  for(i = 0; i < SYSTEM_RAM_LENGTH; i++) {
    SYSTEMRAM[i] = 0;
  }
  for(i = 0x07c0; i < SYSTEM_RAM_LENGTH; i += 128) {
    for(j = i; j < i + 64; j++){
      SYSTEMRAM[j] = 0xff;
    }
  }
}

void systemram_write(uint16_t address, uint8_t value) {
  SYSTEMRAM[address & (SYSTEM_RAM_LENGTH - 1)] = value;
}

uint8_t systemram_read(uint16_t address) {
  return SYSTEMRAM[address & (SYSTEM_RAM_LENGTH - 1)];
}

void m64_ramWrite(uint16_t address, uint8_t value) {
  SYSTEMRAM[address & (SYSTEM_RAM_LENGTH - 1)] = value;
}

uint8_t m64_ramRead(uint16_t address) {
  return SYSTEMRAM[address & (SYSTEM_RAM_LENGTH - 1)];
}


uint8_t *systemram_array() {
  return SYSTEMRAM;
}

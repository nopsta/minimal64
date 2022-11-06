#include "../m64.h"

uint8_t BASICROM[BASIC_ROM_LENGTH];

void m64_setBASICROM(uint8_t *data, uint32_t dataLength) {
  if(dataLength > BASIC_ROM_LENGTH) {
    dataLength = BASIC_ROM_LENGTH;
  }
  memcpy(BASICROM, data, sizeof(uint8_t) * dataLength);
}

uint8_t basicrom_read(uint16_t address) {
  return BASICROM[address & (BASIC_ROM_LENGTH - 1)];
}

void basicrom_write(uint16_t address, uint8_t value) {
  // error!!
}
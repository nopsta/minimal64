#include "../m64.h"

uint8_t CHARROM[CHAR_ROM_LENGTH];

void m64_setCharacterROM(uint8_t *data, uint32_t dataLength) {
  if(dataLength > CHAR_ROM_LENGTH) {
    dataLength = CHAR_ROM_LENGTH;
  }
  memcpy(CHARROM, data, sizeof(uint8_t) * dataLength);
}

uint8_t charrom_read(uint16_t address) {
  return CHARROM[address & (CHAR_ROM_LENGTH - 1)];
}

void charrom_write(uint16_t address, uint8_t value) {
  // error!!
}

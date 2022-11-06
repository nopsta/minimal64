
#include "../m64.h"

#define COLOR_RAM_LENGTH 0x400

uint8_t COLORRAM[COLOR_RAM_LENGTH];

void colorram_reset() {
  int i;
  for(i = 0; i < COLOR_RAM_LENGTH; i++) {
    COLORRAM[i] = 0;
  }
}

uint8_t colorram_read(uint16_t address) {
  return COLORRAM[address & (COLOR_RAM_LENGTH - 1)];
}
void colorram_write(uint16_t address, uint8_t value) {
  COLORRAM[address & (COLOR_RAM_LENGTH - 1)] = value & 0xf;
}

void colorramdisconnectedbus_write(uint16_t address, uint8_t value) {
  colorram_write(address, value);
}

uint8_t colorramdisconnectedbus_read(uint16_t address) {
  return (colorram_read(address) 
  | (disconnectedbus_read(address) & 0xf0));
  
}


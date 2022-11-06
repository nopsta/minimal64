#include "../m64.h"

void disconnectedbus_write(uint16_t address, uint8_t value) {
}

uint8_t disconnectedbus_read(uint16_t address) {
  return vic_getLastReadByte();
}
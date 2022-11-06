#include "../m64.h"

bank_read_function ioReadMap[MEM_MAX_BANKS];
bank_write_function ioWriteMap[MEM_MAX_BANKS];

void io_setBank(uint8_t bank, bank_read_function readFunction, bank_write_function writeFunction) {
  ioReadMap[bank] = readFunction;
  ioWriteMap[bank] = writeFunction;
}

uint8_t io_read(uint16_t address) {
  return (*ioReadMap[(address >> 8) & 0xf])(address);
}

void io_write(uint16_t address, uint8_t value) {
  (*ioWriteMap[(address >> 8) & 0xf])(address, value);
}


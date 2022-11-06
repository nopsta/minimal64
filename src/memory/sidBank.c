#include "../m64.h"

#define SID_DEF_BASE_ADDRESS 0xd400
#define SID_REG_COUNT 32

uint8_t paddleX;
uint8_t paddleY;

uint8_t mousePortEnabled[2];

void sidbank_setMousePortEnabled(int32_t port, uint8_t enabled) {
  if(port < 0 || port > 1) {
    return;
  }
  mousePortEnabled[port] = enabled;
}

void sidbank_setPaddleX(uint8_t value) {
  paddleX = value;
}

void sidbank_setPaddleY(uint8_t value) {
  paddleY = value;
}

void sidbank_reset() {

}

void sidbank_write(uint16_t address, uint8_t value) {
  sid_write(address & (SID_REG_COUNT - 1), value);
}

uint8_t sidbank_read(uint16_t address) {
  address = address & (SID_REG_COUNT - 1);

  if(mousePortEnabled[0] || mousePortEnabled[1]) {
    if(address == 0x19) {
      return paddleX;
    }

    if(address == 0x1a) {
      return paddleY;
    }
  }

  return sid_read(address & (SID_REG_COUNT - 1) );
}


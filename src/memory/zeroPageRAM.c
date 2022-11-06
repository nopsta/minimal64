#include "../m64.h"

// handle read/write to address 0x0 and 0x1
// hand off to system ram for remaining bytes up to 255

// $01 bits 6 and 7 fall-off cycles (1->0), average is about 350 msec
#define M64_CPU_DATA_PORT_FALL_OFF_CYCLES 350000

typedef struct {
  // value written to processor port
  uint8_t dir;
  uint8_t data;

  // value read from processor port
  uint8_t dataRead;

	// State of processor port pins.
	uint8_t dataOut;

  // cycle that should invalidate the unused bits of the data port.
  int64_t dataSetClkBit6;
  int64_t dataSetClkBit7;

  // indicates if the unused bits of the data port are still valid or should be
  // read as 0, 1 = unused bits valid, 0 = unused bits should be 0
  bool_t dataSetBit6;
  bool_t dataSetBit7;

  // indicates if the unused bits are in the process of falling off. 
  bool_t dataFalloffBit6;
  bool_t dataFalloffBit7;

  // Tape motor status. 
  uint8_t oldPortDataOut;
  // Tape write line status.
  uint8_t oldPortWriteBit;

} zero_ram_t;


zero_ram_t ZERORAM;

void zeroram_updateCpuPort() {
  ZERORAM.dataOut = ( (ZERORAM.dataOut & ~ZERORAM.dir) | (ZERORAM.data & ZERORAM.dir) );
  ZERORAM.dataRead =  ((ZERORAM.data | ~ZERORAM.dir) & (ZERORAM.dataOut | 0x17));
  pla_setCpuPort(ZERORAM.dataRead);

  if (0 == (ZERORAM.dir & 0x20)) {
    ZERORAM.dataRead &= 0xdf;
  }

  if ((ZERORAM.dir & ZERORAM.data & 0x20) != ZERORAM.oldPortDataOut) {
    ZERORAM.oldPortDataOut = (ZERORAM.dir & ZERORAM.data & 0x20);
  }

  if (((~ZERORAM.dir | ZERORAM.data) & 0x8) != ZERORAM.oldPortWriteBit) {
    ZERORAM.oldPortWriteBit =  ((~ZERORAM.dir | ZERORAM.data) & 0x8);
  }
}

uint8_t zeroram_read(uint16_t address) {
  if (address == 0) {
    return ZERORAM.dir;
  } else if (address == 1) {
    if (ZERORAM.dataFalloffBit6 || ZERORAM.dataFalloffBit7) {
      if (ZERORAM.dataSetClkBit6 < clock_getTime(&m64_clock, PHASE_PHI2)) {
        ZERORAM.dataFalloffBit6 = false;
        ZERORAM.dataSetBit6 = false;
      }

      if (ZERORAM.dataSetClkBit7 < clock_getTime(&m64_clock, PHASE_PHI2)) {
        ZERORAM.dataSetBit7 = false;
        ZERORAM.dataFalloffBit7 = false;
      }
    }

    return (uint8_t) (
              ZERORAM.dataRead & 0xff 
              - (((!ZERORAM.dataSetBit6 ? 1 : 0) << 6) 
              + ((!ZERORAM.dataSetBit7 ? 1 : 0) << 7)));
	} else {
    return systemram_read(address);
	}
}

void zeroram_write(uint16_t address, uint8_t value) {
  if (address == 0) {
    if (ZERORAM.dataSetBit7 && (value & 0x80) == 0 && !ZERORAM.dataFalloffBit7) {
      ZERORAM.dataFalloffBit7 = true;
      ZERORAM.dataSetClkBit7 = clock_getTime(&m64_clock, PHASE_PHI2) + M64_CPU_DATA_PORT_FALL_OFF_CYCLES;
    }
    if (ZERORAM.dataSetBit6 && (value & 0x40) == 0 && !ZERORAM.dataFalloffBit6) {
      ZERORAM.dataFalloffBit6 = true;
      ZERORAM.dataSetClkBit6 = clock_getTime(&m64_clock, PHASE_PHI2) + M64_CPU_DATA_PORT_FALL_OFF_CYCLES;
    }
    if (ZERORAM.dataSetBit7 && (value & 0x80) != 0 && ZERORAM.dataFalloffBit7) {
      ZERORAM.dataFalloffBit7 = false;
    }
    if (ZERORAM.dataSetBit6 && (value & 0x40) != 0 && ZERORAM.dataFalloffBit6) {
      ZERORAM.dataFalloffBit6 = false;
    }
    ZERORAM.dir = value;
    zeroram_updateCpuPort();
    value = disconnectedbus_read(address);
  } else if (address == 1) {
    if ((ZERORAM.dir & 0x80) != 0 && (value & 0x80) != 0) {
      ZERORAM.dataSetBit7 = true;
    }
    if ((ZERORAM.dir & 0x40) != 0 && (value & 0x40) != 0) {
      ZERORAM.dataSetBit6 = true;
    }
    ZERORAM.data = value;
    zeroram_updateCpuPort();
    value = disconnectedbus_read(address);
  }
  systemram_write(address, value);
}

void zeroram_reset() {
  ZERORAM.oldPortDataOut =  0xff;
  ZERORAM.oldPortWriteBit = 0xff;
  ZERORAM.data = 0x3f;
  ZERORAM.dataOut = 0x3f;
  ZERORAM.dataRead = 0x3f;
  ZERORAM.dir = 0;
  ZERORAM.dataSetBit6 = false;
  ZERORAM.dataSetBit7 = false;
  ZERORAM.dataFalloffBit6 = false;
  ZERORAM.dataFalloffBit7 = false;
  zeroram_updateCpuPort();
}

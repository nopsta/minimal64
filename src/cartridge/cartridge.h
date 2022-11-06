
#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "../m64.h" 

#define CARTRIDGE_NULL          -1
#define CARTRIDGE_NORMAL        0
#define CARTRIDGE_OCEAN_TYPE_1  5
#define CARTRIDGE_C64GS         15
#define CARTRIDGE_MAGIC_DESK    19

struct cartridge {
  int32_t type;

  bool_t nmiState;
  bool_t irqState;

  // Cartridge ROM LO Banks
  bool_t  has_roml;
  int32_t romlbank;
  int32_t romlbank_count;
  uint8_t *roml[64];

  // Cartridge ROM HI Banks
  bool_t  has_romh;
  int32_t romhbank;
  int32_t romhbank_count;
  uint8_t *romh[16];

  // Cartridge game and exrom flags
  bool_t game;
  bool_t exrom;
};


typedef struct cartridge cartridge_t;

void cartridge_init();

int32_t cartridge_read(uint8_t *data, uint32_t dataLength);


uint8_t cartridge_romlRead(uint16_t address);
void cartridge_romlWrite(uint16_t address, uint8_t value);
uint8_t cartridge_romhRead(uint16_t address);
void cartridge_romhWrite(uint16_t address, uint8_t value);
uint8_t cartridge_ultimaxRead(uint16_t address);
void cartridge_ultimaxWrite(uint16_t address, uint8_t value);


void cartridge_io1Write(uint16_t address, uint8_t value);
uint8_t cartridge_io1Read(uint16_t address);
void cartridge_io2Write(uint16_t address, uint8_t value);
uint8_t cartridge_io2Read(uint16_t address);

void cartridge_reset();
void cartridge_setNMI(bool_t state);
void cartridge_setIRQ(bool_t state);

#endif
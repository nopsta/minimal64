#include "../m64.h"

cartridge_t m64_cartridge;

// allocate memory for cartridge roml banks
// called when reading in a cartridge
void cartridge_setRomlBankCount(int32_t count) {
  int32_t i;

  if(count < m64_cartridge.romlbank_count) {
    // can free memory
    for(i = count; i < m64_cartridge.romlbank_count; i++) {
      free(m64_cartridge.roml[i]);
    }
  }

  for(i = m64_cartridge.romlbank_count; i < count; i++) {
    m64_cartridge.roml[i] = malloc(sizeof(uint8_t) * 0x2000);
  }

  m64_cartridge.romlbank_count = count;
}

// allocate memory for cartridge romh banks
// called when reading in a cartridge
void cartridge_setRomhBankCount(int32_t count) {
  int32_t i;

  // allocate memory, if count is zero, no need to allocate memory
  if(count < m64_cartridge.romhbank_count) {
    // can free memory
    for(i = count; i < m64_cartridge.romhbank_count; i++) {
      free(m64_cartridge.romh[i]);
    }
  }

  for(i = m64_cartridge.romhbank_count; i < count; i++) {
    m64_cartridge.romh[i] = malloc(sizeof(uint8_t) * 0x2000);
  }

  m64_cartridge.romhbank_count = count;
}

// used for bank switching
void cartridge_io1Write(uint16_t address, uint8_t value) {
  if(m64_cartridge.type == CARTRIDGE_NULL) {
    return;
  }

  int32_t bank = 0;
  switch(m64_cartridge.type) {
    case CARTRIDGE_NORMAL:
      return;

    case CARTRIDGE_OCEAN_TYPE_1:
      if (address == 0xde00) {
        // Bank switching is done by writing to $DE00. The lower six bits give the
        // bank number (ranging from 0-63). Bit 8 in this selection word is always
        // set.
        bank = value & 0x3f;
        m64_cartridge.romlbank = bank;
      }
      break;
    case CARTRIDGE_C64GS:
      // Bank switching is done by writing to address $DE00+X, where  X  is  the
      // bank number (STA $DE00,X)
      bank = address & 0x3f;
      m64_cartridge.romlbank = bank;
    break;
    case CARTRIDGE_MAGIC_DESK:
      if (address == 0xde00) {
        // Bank switching is done by writing the bank number to $DE00. Deviant from the
        // Ocean type, bit 8 is cleared for selecting one of the ROM banks. If bit
        // 8 is set ($DE00 = $80), the GAME/EXROM lines are disabled,  turning  on
        // RAM at $8000-$9FFF instead of ROM.

        m64_cartridge.romlbank = value & 0x1f;
        pla_setGameExrom(true, (value & 0x80) != 0, true, (value & 0x80) != 0);
      }
      break;
  }
}

uint8_t cartridge_io1Read(uint16_t address) {
  if(m64_cartridge.type == CARTRIDGE_NULL) {
    return disconnectedbus_read(address);
  }
  switch(m64_cartridge.type) {
    case CARTRIDGE_C64GS:
      m64_cartridge.romlbank = 0;
      return 0;
    break;
  }
  return disconnectedbus_read(address);
}

void cartridge_io2Write(uint16_t address, uint8_t value) {
  if(m64_cartridge.type == CARTRIDGE_NULL) {
    return;
  }
  return;
}

uint8_t cartridge_io2Read(uint16_t address) {
  if(m64_cartridge.type == CARTRIDGE_NULL) {
    return disconnectedbus_read(address);
  }
  return disconnectedbus_read(0);
}

void cartridge_readNormal(uint8_t *data, uint32_t dataLength) {

  uint8_t *chip_packet = &(data[0x40]);

  int32_t rom_size_h = chip_packet[0xe];
  int32_t rom_size_l = chip_packet[0xf];
  int32_t i;


  m64_cartridge.romlbank = 0;
  m64_cartridge.romhbank = 0;

  // each rom bank is 0x2000 in size (8192 bytes)

  if(rom_size_h == 0x10) {
    // 4k cartridge
    cartridge_setRomhBankCount(1);
    cartridge_setRomlBankCount(0);


    for(i = 0; i < 0x1000; i++) {
      m64_cartridge.romh[m64_cartridge.romhbank][i] = chip_packet[0x10 + i];
      m64_cartridge.romh[m64_cartridge.romhbank][i + 0x1000] = chip_packet[0x10 + i];
    }
    m64_cartridge.has_roml = false;
    m64_cartridge.has_romh = true;
  } else if(rom_size_h == 0x20) {

    // 8k cartridge
    cartridge_setRomhBankCount(1);
    cartridge_setRomlBankCount(0);
    for(i = 0; i < 0x2000; i++) {
      m64_cartridge.roml[m64_cartridge.romlbank][i] = chip_packet[0x10 + i];
    }

    m64_cartridge.has_roml = true;
    m64_cartridge.has_romh = false;

  } else if(rom_size_h == 0x40) {
    // 16k cartridge
    cartridge_setRomhBankCount(1);
    cartridge_setRomlBankCount(1);

    for(i = 0; i < 0x2000; i++) {
      m64_cartridge.roml[m64_cartridge.romlbank][i] = chip_packet[0x10 + i];
      m64_cartridge.romh[m64_cartridge.romhbank][i] = chip_packet[0x10 + i + 0x2000];
    }

    m64_cartridge.has_roml = true;
    m64_cartridge.has_romh = true;

  }

}

// Ocean Type 1
// Size - 128Kb, 256Kb or 512Kb sizes (16, 32 or 64 banks of 8Kb each)
// GAME - inactive (0)
// EXROM - inactive (0)
//  Load address - Banks 00-15 - $8000-9FFF
//                 Banks 16-31 - $A000-BFFF  (except Terminator 2, 512k banks 00 - 63 $8000 - $9fff)

void cartridge_readOceanType1(uint8_t *data, uint32_t dataLength) {
  int32_t packet_offset = 0x40;
  int32_t i = 0;

  int32_t bank_count_l = 0;
  int32_t bank_count_h = 0;

  while(packet_offset < dataLength) {
    uint8_t *chip_packet = &(data[packet_offset]);
    int32_t bank = chip_packet[0xb];

    if(bank < 16) {
      if(bank + 1 > bank_count_l) {
        bank_count_l = bank + 1;
      }
    } else {
      bank -= 16;
      if(bank + 1 > bank_count_h) {
        bank_count_h = bank + 1;
      }
    }

    packet_offset += 0x2010;
  }

  if(bank_count_l + bank_count_h == 64) {
    // its a 512k cart
    bank_count_l = 64;
    bank_count_h = 0;
    m64_cartridge.has_romh = false;
    m64_cartridge.game = true;
  }

  cartridge_setRomlBankCount(bank_count_l);
  cartridge_setRomhBankCount(bank_count_h);


  packet_offset = 0x40;
  while(packet_offset < dataLength) {
    uint8_t *chip_packet = &(data[packet_offset]);
    int32_t bank = chip_packet[0xb];

    if(bank < 16 || bank_count_l == 64) {
      m64_cartridge.has_roml = true;
      for(i = 0; i < 0x2000; i++) {
        m64_cartridge.roml[bank][i] = chip_packet[0x10 + i];
      }
    } else {
      m64_cartridge.has_romh = true;
      for(i = 0; i < 0x2000; i++) {
        m64_cartridge.romh[bank - 16][i] = chip_packet[0x10 + i];
      }
    }

    packet_offset += 0x2010;
  }

}

void cartridge_readM64GS(uint8_t *data, uint32_t dataLength) {

  int32_t packet_offset = 0x40;
  int32_t i = 0;
  m64_cartridge.has_roml = true;
  m64_cartridge.has_romh = false;

  int32_t bank_count = 0;
  // find the bank count
  while(packet_offset < dataLength) {
    uint8_t *chip_packet = &(data[packet_offset]);
    int32_t bank = chip_packet[0xb];
    if(bank + 1 > bank_count) {
      bank_count = bank + 1;
    }
    packet_offset += 0x2010;
  }

  cartridge_setRomlBankCount(bank_count);
  cartridge_setRomhBankCount(0);

  packet_offset = 0x40;
  while(packet_offset < dataLength) {
    uint8_t *chip_packet = &(data[packet_offset]);
    int32_t bank = chip_packet[0xb];
    for(i = 0; i < 0x2000; i++) {
      m64_cartridge.roml[bank][i] = chip_packet[0x10 + i];
    }

    packet_offset += 0x2010;
  }
}



void cartridge_readMagicDesk(uint8_t *data, uint32_t dataLength) {

  int32_t packet_offset = 0x40;
  int32_t i = 0;
  m64_cartridge.has_roml = true;
  m64_cartridge.has_romh = false;

  int32_t bank_count = 0;
  while(packet_offset < dataLength) {
    uint8_t *chip_packet = &(data[packet_offset]);
    int32_t bank = chip_packet[0xb];
    if(bank + 1 > bank_count) {
      bank_count = bank + 1;
    }
    packet_offset += 0x2010;
  }


  cartridge_setRomlBankCount(bank_count);
  cartridge_setRomhBankCount(0);

  packet_offset = 0x40;
  while(packet_offset < dataLength) {
    uint8_t *chip_packet = &(data[packet_offset]);
    int32_t bank = chip_packet[0xb];
    for(i = 0; i < 0x2000; i++) {
      m64_cartridge.roml[bank][i] = chip_packet[0x10 + i];
    }

    packet_offset += 0x2010;
  }
}



void cartridge_init() {
  m64_cartridge.type = CARTRIDGE_NULL;
  m64_cartridge.romlbank_count = 0;
  m64_cartridge.romhbank_count = 0;
}


// load a cartridge
int32_t cartridge_read(uint8_t *data, uint32_t dataLength) {
  // see notes/crt-format.txt
  bool_t knownFormat = false;

  m64_cartridge.exrom = data[0x18] == 1;
  m64_cartridge.game = data[0x19] == 1;
  m64_cartridge.romlbank = 0;
  m64_cartridge.romhbank = 0;


  if(data[0x16] == 0) {
    switch(data[0x17]) {
      case 0:
        knownFormat = true;
        m64_cartridge.type = CARTRIDGE_NORMAL;
        cartridge_readNormal(data, dataLength);

        break;
      case 5:
        knownFormat = true;
        m64_cartridge.type = CARTRIDGE_OCEAN_TYPE_1;
        cartridge_readOceanType1(data, dataLength);
        break;
      case 15:
        knownFormat = true;
        m64_cartridge.type = CARTRIDGE_C64GS;
        cartridge_readM64GS(data, dataLength);
        break;
      case 19:

        knownFormat = true;
        m64_cartridge.type = CARTRIDGE_MAGIC_DESK;
        cartridge_readMagicDesk(data, dataLength);
        break;
    }
  }

  return knownFormat ? 1 : 0;

}

uint8_t cartridge_romlRead(uint16_t address) {
  if(m64_cartridge.type == CARTRIDGE_NORMAL && m64_cartridge.has_roml) {
    return m64_cartridge.roml[m64_cartridge.romlbank][address & 0x1fff];
  }
  if(m64_cartridge.type == CARTRIDGE_C64GS && m64_cartridge.has_roml) {
    return m64_cartridge.roml[m64_cartridge.romlbank][address & 0x1fff];
  }

  if(m64_cartridge.type == CARTRIDGE_OCEAN_TYPE_1 && m64_cartridge.has_roml) {
    // banks in the lower 128KB are mapped to $8000-$9fff
    // banks in the upper 128KB are mapped to $a000-$bfff
    if(m64_cartridge.romlbank < m64_cartridge.romlbank_count) {
      return m64_cartridge.roml[m64_cartridge.romlbank][address & 0x1fff];
    } else {
      return m64_cartridge.romh[m64_cartridge.romlbank - 16][address & 0x1fff];
    }
  }

  if(m64_cartridge.type == CARTRIDGE_MAGIC_DESK && m64_cartridge.has_roml) {
    return m64_cartridge.roml[m64_cartridge.romlbank][address & 0x1fff];
  }

  return disconnectedbus_read(address);
}

void cartridge_romlWrite(uint16_t address, uint8_t value) {
  disconnectedbus_write(address, value);
}

uint8_t cartridge_romhRead(uint16_t address) {
  if(m64_cartridge.type == CARTRIDGE_NORMAL && m64_cartridge.has_romh) {
    return m64_cartridge.romh[m64_cartridge.romhbank][address & 0x1fff];
  }

  if(m64_cartridge.type == CARTRIDGE_OCEAN_TYPE_1 && m64_cartridge.has_roml) {
    // banks in the lower 128KB are mapped to $8000-$9fff
    // banks in the upper 128KB are mapped to $a000-$bfff
    if(m64_cartridge.romlbank < m64_cartridge.romlbank_count) {
      return m64_cartridge.roml[m64_cartridge.romlbank][address & 0x1fff];
    } else {
      return m64_cartridge.romh[m64_cartridge.romlbank - 16][address & 0x1fff];
    }

  }

  return disconnectedbus_read(address);
}

void cartridge_romhWrite(uint16_t address, uint8_t value) {
  disconnectedbus_write(address, value);
}

uint8_t cartridge_ultimaxRead(uint16_t address) {
  return disconnectedbus_read(address);
}

void cartridge_ultimaxWrite(uint16_t address, uint8_t value) {
  disconnectedbus_write(address, value);
}

void cartridge_reset() {
  m64_cartridge.nmiState = false;
  m64_cartridge.irqState = false;

  if(m64_cartridge.type == CARTRIDGE_NULL) {
    return;
  }

  if(m64_cartridge.type == CARTRIDGE_C64GS) {
    // set bank
    cartridge_io1Write(0xde00, 0x00);
  }

  if(m64_cartridge.type == CARTRIDGE_OCEAN_TYPE_1) {
    // set bank
    cartridge_io1Write(0xde00, 0x00);
  }

  if(m64_cartridge.type == CARTRIDGE_MAGIC_DESK) {
    // set bank
    cartridge_io1Write(0xde00, 0x00);
  }

  pla_setGameExrom(m64_cartridge.game, m64_cartridge.exrom, m64_cartridge.game, m64_cartridge.exrom);
}

void cartridge_setNMI(bool_t state) {
  if (state != m64_cartridge.nmiState) {
    pla_setNMI(state);
    m64_cartridge.nmiState = state;
  }
}

void cartridge_setIRQ(bool_t state) {
  if (state != m64_cartridge.irqState) {
    pla_setIRQ(state);
    m64_cartridge.irqState = state;
  }
}


#include "../m64.h"

struct pla {

  // **** CPU Control Lines **** //

  // LORAM is a control line which banks the 8 kByte BASIC ROM in or out of the CPU address space.
  // Normally, this line is logically high (set to 1) for BASIC operation
  // pin I1, connected to LORAM on IO port of CPU
  bool_t LORAM;


  // HIRAM (bit 1, weight 2) is a control line which banks the 8 kByte KERNAL ROM in or out of the CPU address space. 
  // Normally, this line is logically high (set to 1) for KERNAL ROM operation
  // pin I2, connected to HIRAM on IO port of CPU
  bool_t HIRAM;

  // pin I3, conencted to CHAREN on IO port of CPU
  // CHAREN (bit 2, weight 4) is a control line which banks the 4 kByte character generator ROM in or out of the CPU address space.
  // From the CPU point of view, the character generator ROM occupies the same address space as the I/O devices ($D000-$DFFF). 
  // When the CHAREN line is set to 1 (as is normal), the I/O devices appear in the CPU address space, and the character generator ROM is not accessible
  bool_t CHAREN;

  // vic memory base: $0000, $4000, $8000, $c000.
  // The two address lines VA13 and VA12 are directly connected to the VIC-II. These lines
  // are needed to address the 16k address space of the VIC-II. Note that #VA14 is from a
  // completely different source, it is connected to a CIA output.
  // pins I15 and I14 connected to VA12 and VA 13 on VIC-II
  uint16_t vicMemBase;


  // separate the 64kb address space into 4k chunks
  // make maps of functions for the CPU and VIC to determine 
  // where they read/write for the 4k chunk
  // when reading/writing the mapped bank_read/bank_write function will be called
  bank_read_function cpuReadMap[MEM_MAX_BANKS];
  bank_write_function cpuWriteMap[MEM_MAX_BANKS];

  bank_read_function vicReadMap[MEM_MAX_BANKS];
  bank_write_function vicWriteMap[MEM_MAX_BANKS];

  // pla pin I9 connected to BA (bus available) on the VIC-II
  // If BA is set high we have normal operations and the CPU 
  // knows it can do its read/write access to the data bus during a high ϕ2 phase. 
  // If the VIC-II needs more cycles because it is 
  // for instance fetching Character Pointers on every 8th line, 
  // BA is set low and by that RDY on the CPU side becomes low as well.
  // if ba is false, vic wants to use the bus
  bool_t ba;

  // pin I10 connected to inverted version of AEC (Address Enable Control) on the VIC-II
  // the AEC line signals the current phase within ϕ2. 
  // if aec is true on phi2, cpu is reading from the bus
  bool_t aec;

  // pin I13 connected to #GAME on pin 8 of cartridge port  
  bool_t gamePHI1;
  bool_t gamePHI2;

  // pin I12 connected #EXROM on pin 9 of cartridge port
  bool_t exromPHI1;
  bool_t exromPHI2;


  // can only trigger nmi is nmiCount is 0
  uint32_t nmiCount;

  // can only trigger irq is irqCount is 0
  uint32_t irqCount;

  bool_t cartridgeDma;

  m6510_t *cpu;

  event_t aecDisableEvent;
};

pla_t PLA;


/**
 * Event to change the BA state
 * called by setBA if setting to false
 * event is scheduled for 3 cycles time
 * on phi 1
 * 
 * when the vic requires more cycles: This is the case when the VIC accesses the character pointers
 * and the sprite data. In the first case it needs 40 additional cycles, in
 * the second case it needs 2 cycles per sprite. BA will then go low 3 cycles
 * before the VIC takes over the bus completely (3 cycles is the maximum
 * number of successive write accesses of the 6510). After 3 cycles, AEC stays
 * low during the second clock phase so that the VIC can output its addresses.
 */
void pla_aecDisableEventFunction(void *context) {
  PLA.aec = false;
}

void pla_init() {
  int i;

  PLA.LORAM = true;
  PLA.HIRAM = true;
  PLA.CHAREN = true;

  PLA.vicMemBase = 0;

  PLA.ba = false;
  PLA.aec = false;

  PLA.gamePHI1 = false;
  PLA.gamePHI2 = false;

  PLA.exromPHI1 = false;
  PLA.exromPHI2 = false;

  PLA.nmiCount = 0;
  PLA.irqCount = 0;

  PLA.cartridgeDma = false;
  io_setBank(0, &vic_read, &vic_write);
  io_setBank(1, &vic_read, &vic_write);
  io_setBank(2, &vic_read, &vic_write);
  io_setBank(3, &vic_read, &vic_write);


  io_setBank(4, &sidbank_read, &sidbank_write);
  io_setBank(5, &sidbank_read, &sidbank_write);
  io_setBank(6, &sidbank_read, &sidbank_write);
  io_setBank(7, &sidbank_read, &sidbank_write);

  io_setBank(8, &colorramdisconnectedbus_read, &colorramdisconnectedbus_write);
  io_setBank(9, &colorramdisconnectedbus_read, &colorramdisconnectedbus_write);
  io_setBank(10, &colorramdisconnectedbus_read, &colorramdisconnectedbus_write);
  io_setBank(11, &colorramdisconnectedbus_read, &colorramdisconnectedbus_write);

  io_setBank(12, &cia1_read, &cia1_write);
  io_setBank(13, &cia2_read, &cia2_write);


  io_setBank(14, &disconnectedbus_read, &disconnectedbus_write);
  io_setBank(15, &disconnectedbus_read, &disconnectedbus_write);

  PLA.cpuReadMap[0] = &zeroram_read;

  PLA.cpuWriteMap[0] = &zeroram_write;


  PLA.vicReadMap[0] = &systemram_read;
  PLA.vicWriteMap[0] = &systemram_write;

  for(i = 1; i < MEM_MAX_BANKS; i++) {
    PLA.cpuReadMap[i] = &systemram_read;

    PLA.cpuWriteMap[i] = &systemram_write;

    PLA.vicReadMap[i] = &systemram_read;
    PLA.vicWriteMap[i] = &systemram_write;
  }

  PLA.aecDisableEvent.event = &pla_aecDisableEventFunction;
}



// ------------------- CPU ------------------------- //

// see table here: https://www.c64-wiki.com/wiki/Bank_Switching
// LORAM is basic
// HIRAM is kernal
// CHAREN is io
void pla_updateCPUMaps() {

  int i;

  if (PLA.exromPHI2 && !PLA.gamePHI2) {
    // 16K Cartridge, $8000-$9FFF / $E000-$FFFF (ROML / ROMH). Ultimax mode.
    // GAME = 0, EXROM = 1  

    for(i = 1; i < MEM_MAX_BANKS; i++) {
      PLA.cpuReadMap[i] = &cartridge_ultimaxRead;
      
      PLA.cpuWriteMap[i] = &cartridge_ultimaxWrite;
    }

    // cartridge_roml
    PLA.cpuReadMap[8] = &cartridge_romlRead;
    PLA.cpuReadMap[9] = &cartridge_romlRead;

    PLA.cpuWriteMap[8] = &cartridge_romlWrite;
    PLA.cpuWriteMap[9] = &cartridge_romlWrite;

    PLA.cpuReadMap[13] = &io_read;
    PLA.cpuWriteMap[13] = &io_write;

    // cartridge_romh
    PLA.cpuReadMap[14] = &cartridge_romhRead;
    PLA.cpuReadMap[15] = &cartridge_romhRead;

    PLA.cpuWriteMap[14] = &cartridge_romhWrite;
    PLA.cpuWriteMap[15] = &cartridge_romhWrite;
  } else {
    for(i = 1; i < MEM_MAX_BANKS; i++) {
      PLA.cpuReadMap[i] = &systemram_read;

      PLA.cpuWriteMap[i] = &systemram_write;
    }

    if (PLA.LORAM && PLA.HIRAM && !PLA.exromPHI2) {
      // cartridge_roml
      PLA.cpuReadMap[8] = &cartridge_romlRead;
      PLA.cpuReadMap[9] = &cartridge_romlRead;

    }

    if (PLA.HIRAM && !PLA.exromPHI2 && !PLA.gamePHI2) {
      // 16K Cartridge, $8000-$9FFF / $A000-$BFFF (ROML / ROMH).
      // GAME = 0, EXROM = 0
      // ROML/ROMH are read only, Basic ROM is overwritten by ROMH.        
      PLA.cpuReadMap[10] = &cartridge_romhRead;
      PLA.cpuReadMap[11] = &cartridge_romhRead;
      //PLA.cartridge.getRomh();
    } else if (PLA.LORAM && PLA.HIRAM && PLA.gamePHI2) {
      PLA.cpuReadMap[10] = &basicrom_read;
      PLA.cpuReadMap[11] = &basicrom_read;
    }

    if ( PLA.CHAREN && (PLA.LORAM || PLA.HIRAM) && (PLA.gamePHI2 || (!PLA.gamePHI2 && !PLA.exromPHI2) )) {
      PLA.cpuReadMap[13] = &io_read;
      PLA.cpuWriteMap[13] = &io_write;
    } else if (!PLA.CHAREN && ( ((PLA.LORAM || PLA.HIRAM) && PLA.gamePHI2) 
                || (PLA.HIRAM && !PLA.gamePHI2 && !PLA.exromPHI2)  )) {
      PLA.cpuReadMap[13] = &charrom_read;
    } 

    if (PLA.HIRAM && (PLA.gamePHI2 || (!PLA.gamePHI2 && !PLA.exromPHI2) )) {
      PLA.cpuReadMap[14] = &kernal_read;
      PLA.cpuReadMap[15] = &kernal_read;
    } 
  }
  
  io_setBank(14, &cartridge_io1Read, &cartridge_io1Write);
  io_setBank(15, &cartridge_io2Read, &cartridge_io2Write);
}

void pla_updateVICMaps() {
  int i;

  if (PLA.gamePHI1 || (!PLA.gamePHI1 && !PLA.exromPHI1) ) {
    PLA.vicReadMap[1] = &charrom_read;
    PLA.vicWriteMap[1] = &charrom_write;

    PLA.vicReadMap[9] = &charrom_read;
    PLA.vicWriteMap[9] = &charrom_write;

  } else {
    PLA.vicReadMap[1] = &systemram_read;
    PLA.vicWriteMap[1] = &systemram_write;

    PLA.vicReadMap[9] = &systemram_read;
    PLA.vicWriteMap[9] = &systemram_write;
  }

  if (PLA.exromPHI1 && !PLA.gamePHI1) {
    for (i = 3; i < MEM_MAX_BANKS; i += 4) {
      PLA.vicReadMap[1] = &cartridge_romhRead;
      PLA.vicWriteMap[1] = &cartridge_romhWrite;
    }
  } else {
    for (i = 3; i < MEM_MAX_BANKS; i += 4) {
      PLA.vicReadMap[i] = &systemram_read;
      PLA.vicWriteMap[i] = &systemram_write;
    }
  }
}


void pla_reset() {
  PLA.vicMemBase = 0;
  PLA.aec = false;
  PLA.ba = true;

  PLA.gamePHI1 = true;
  PLA.gamePHI2 = true;

  PLA.exromPHI1 = true;
  PLA.exromPHI2 = true;

  PLA.nmiCount = 0;
  PLA.irqCount = 0;
  sidbank_reset();
  colorram_reset();

  cartridge_reset();

  pla_updateVICMaps();
  pla_updateCPUMaps();
}


void  pla_setCpuPort(uint8_t state) {
  PLA.LORAM = (state & 1) != 0;
  PLA.HIRAM = (state & 2) != 0;
  PLA.CHAREN = (state & 4) != 0;
  pla_updateCPUMaps();
}



uint8_t m64_cpuRead(uint16_t address) {
  return (PLA.cpuReadMap[address >> 12])(address);
}

void m64_cpuWrite(uint16_t address, uint8_t value) {
  (PLA.cpuWriteMap[address >> 12])(address, value);
}


uint8_t pla_cpuRead(uint16_t address) {
  return (PLA.cpuReadMap[address >> 12])(address);
}

void pla_cpuWrite(uint16_t address, uint8_t value) {
  (PLA.cpuWriteMap[address >> 12])(address, value);
}

void pla_setVicMemBase(uint16_t base) {
  PLA.vicMemBase = base;
}


// Access memory in PHI1 as seen by VIC. The address should only contain the bottom
uint8_t pla_vicReadMemoryPHI1(uint16_t addr) {
  // VIC can read memory in PHI 1 with no problems

  addr |= PLA.vicMemBase;

  uint8_t result =  (PLA.vicReadMap[addr >> 12])(addr);

  return result;
}

// Access memory in PHI 2 as seen by VIC. 
// The address should only contain the bottom 14 bits.
uint8_t pla_vicReadMemoryPHI2(uint16_t addr) {
  if (PLA.aec) {
    // VIC trying to read from the bus in phi2, but AEC is high
    // If AEC (Address Enable Control) is still high (CPU is connected to the bus), the 0xff read is
    // emulated, as the VIC has tristated itself from the bus
    return 0xff;
  }

  // read is same as a PHI 1 read
  return pla_vicReadMemoryPHI1(addr);
}


// Access color RAM from VIC. The address should be between 0 - 0x3ff.
uint8_t pla_vicReadColorMemoryPHI2(uint16_t addr) {
  if (PLA.aec) {
    // If AEC (Address Enable Control) is still high, the bottom 4 bits of the value CPU is stalled on
    // reading will be acquired instead.  if aec is true on phi2, cpu is reading from the bus
    return (m6510_getStalledOnByte(PLA.cpu) & 0xf);
  } else {
    return colorram_read(addr);
  }
}


void pla_setCpu(m6510_t *cpu) {
  PLA.cpu = cpu;
}

m6510_t *pla_getCpu() {
  return PLA.cpu;
}

// game and exrom are used for expansion port
void  pla_setGameExrom(bool_t gamephi1, bool_t exromphi1, bool_t gamephi2, bool_t exromphi2) {
  PLA.gamePHI1 = gamephi1;
  PLA.exromPHI1 = exromphi1;
  pla_updateVICMaps();

  PLA.exromPHI2 = exromphi2;
  PLA.gamePHI2 = gamephi2;

  pla_updateCPUMaps();
}

// set the Bus Available (BA) flag
// called by the VIC when it wants to use the bus in PHI 2 (taking it away from CPU)
// call permitted in PHI 1
void pla_setBA(bool_t state) {
  // return if state isn't changing
  if(state == PLA.ba) {
    return;
  }

  PLA.ba = state;

  if (!PLA.cartridgeDma) {
    m6510_setRDY(PLA.cpu, state);
  }
  
  if (state) {
    PLA.aec = true;
    clock_cancelEvent(&m64_clock, &PLA.aecDisableEvent);
  } else {
    clock_scheduleEvent(&m64_clock, &PLA.aecDisableEvent, 3, PHASE_PHI1);
  }
}

// called by chips to trigger a NMI
// usually triggered on phi 1
void pla_setNMI(bool_t state) {
  if (state) {
    if (PLA.nmiCount == 0) {
      m6510_triggerNMI(PLA.cpu);
    }
    PLA.nmiCount++;
  } else {
    PLA.nmiCount--;
  }
}

// called by chips to trigger an IRQ
// usually triggered on phi 1
void pla_setIRQ(bool_t state) {
  if (state) {
    if (PLA.irqCount == 0) {
      m6510_triggerIRQ(PLA.cpu);
    }
    PLA.irqCount++;
  } else {
    PLA.irqCount--;
    if (PLA.irqCount == 0) {
      m6510_clearIRQ(PLA.cpu);
    }
  }
}




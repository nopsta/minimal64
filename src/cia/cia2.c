/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For the full
 * license text, see http://www.gnu.org/licenses/gpl.html.
 * 
 * Author: nopsta 2022
 * 
 * Based on parts of: 
 * MOS6526.java from JSIDPlay2: Antti S. Lankila (alankila@bel.fi)
 * SIDPLAY2 Copyright (©) 2001-2004 Simon White <sidplay2@yahoo.com>
 * VICE Copyright (©) 2009 VICE Project
 * 
 * See the cia/notes directory from more about the cia chip
 * 
 */

#include "../m64.h"


m6526_t cia2;

// if IO is mapped in, this is the function mapped to write to CIA2 register addresses
// see memory/pla.c 
void cia2_write(uint16_t reg, uint8_t data) {
  // just let the m6526 function handle it  
  m6526_write(&cia2, reg, data);
}

// if I/O is banked in, this is the function mapped to read from CIA2 register addresses 
// see memory/pla.c 
uint8_t cia2_read(uint16_t reg) {
  // just let the m6526 function handle it  
  return m6526_read(&cia2, reg);
}

// CIA 2 Interrupts are NMI interrupts
void cia2_interrupt(m6526_t *m6526, bool_t state) {
  pla_setNMI(state);
}

/*
  CIA 2 Data Port A (PRA) $dd00 Write
  // see notes/io-ports.txt  
*/
void cia2_writePRA(m6526_t *m6526, uint8_t data) {
  // Bit 0..1: Select the position of the VIC-memory
  pla_setVicMemBase( (~data & 3) << 14);

  // Bit 2: RS-232: TXD Output, userport: Data PA 2 (pin M)
  // Userport/RS-232 not implemented

  // Bit 3..5: serial bus Output (0=High/Inactive, 1=Low/Active)
  // Bit 6..7: serial bus Input (0=Low/Active, 1=High/Inactive)
  iecBus_writeToIECBus(~data);
}


/*
  CIA 2 Data Port A (PRA) $dd00 Read
  // see notes/io-ports.txt  
*/
uint8_t cia2_readPRA(m6526_t *m6526) {
  // Bit 3..5: serial bus Output (0=High/Inactive, 1=Low/Active)
  // Bit 6..7: serial bus Input (0=Low/Active, 1=High/Inactive)
  return iecBus_readFromIECBus() | 0x3f;
}

/*
  CIA 2 Data Port B (PRB) $dd01 Write
  // see notes/io-ports.txt
*/
// Bit 0..7: userport Data PB 0-7 (Pins C,D,E,F,H,J,K,L)
void cia2_writePRB(m6526_t *m6526, uint8_t data) {
  // user port/RS 232 not implemented -- 
}

/*
  CIA 2 Data Port B (PRB) $dd01 Read
  // see notes/io-ports.txt
*/
uint8_t cia2_readPRB(m6526_t *m6526) {
  // user port/RS 232 not implemented -- 
  return 0xff;
}

void cia2_pulse(m6526_t *m6526) {

}

void cia2_init(uint32_t model) {
  // setup function pointers for cia 2
  cia2.readPRA = &cia2_readPRA;
  cia2.readPRB = &cia2_readPRB;

  cia2.writePRA = &cia2_writePRA;
  cia2.writePRB = &cia2_writePRB;

  cia2.interrupt = &cia2_interrupt;
  cia2.pulse = &cia2_pulse;

  m6526_init(&cia2, model);
}
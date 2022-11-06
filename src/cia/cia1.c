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

m6526_t cia1;


// if I/O is banked in, this is the function mapped to write to CIA1 register addresses
// see pla.c 
void cia1_write(uint16_t reg, uint8_t data) {
  // just let the m6526 function handle it
  m6526_write(&cia1, reg, data);
}


// if I/O is banked in, this is the function mapped to read from CIA1 register addresses
// see pla.c 
uint8_t cia1_read(uint16_t reg) {
  // just let the m6526 function handle it  
  return m6526_read(&cia1, reg);
}

// CIA 1 Interrupts are IRQ interrupts
void cia1_interrupt(m6526_t *m6526, bool_t state) {
  pla_setIRQ(state);
}

//  CIA 1 Data Port A (PRA) $dc00 Write
// see notes/02-io-ports.txt
void cia1_writePRA(m6526_t *m6526, uint8_t data) {
  // nothing specific to cia 1 when writing
}

//  CIA 1 Data Port B (PRB) $dc01 Write
// see notes/02-io-ports.txt
void cia1_writePRB(m6526_t *m6526, uint8_t data) {

  // bit 4
  if((data & 0x10) == 0) {
    vic_triggerLightpen();
  } else {
    vic_clearLightpen();
  }
}

/*
  CIA 1 Data Port A (PRA) $dc00 Read
  see notes/02-io-ports.txt

  Read/Write: Bit 0..7 keyboard matrix columns
  Read: Joystick Port 2: Bit 0..3 Direction (Left/Right/Up/Down), Bit 4 Fire button. 0 = activated.
  Read: Lightpen: Bit 4 (as fire button), connected also with "/LP" (Pin 9) of the VIC
  Read: Paddles: Bit 2..3 Fire buttons, 
  Read: Bit 6..7 Switch control port 1 (%01=Paddles A) or 2 (%10=Paddles B)
*/
uint8_t cia1_readPRA(m6526_t *m6526) {
  // Read Port B to send to keyboard read column
  uint8_t prbOut = (m6526->regs[M6526_REG_PRB] | ~m6526->regs[M6526_REG_DDRB]);
  prbOut &= joystick_getValue(&(m64_joysticks[0]));

  // read keyboard column
  uint8_t kbd = keyboard_readColumn(prbOut);

  // read the port 2 joystick
  uint8_t joy = joystick_getValue(&(m64_joysticks[1]));

  // Port A is keyboard column anded with joystick port 2
  return (kbd & joy);
}


/*
  CIA 1 Data Port B (PRB) $dc01 Read
  see notes/02-io-ports.txt

  Read/Write: Bit 0..7 keyboard matrix rows
  Read: Joystick Port 1: Bit 0..3 Direction (Left/Right/Up/Down), Bit 4 Fire button. 0 = activated.
  Read: Bit 6: Timer A: Toggle/Impulse output (see register 14 bit 2)
  Read: Bit 7: Timer B: Toggle/Impulse output (see register 15 bit 2)
*/
uint8_t cia1_readPRB(m6526_t *m6526) {
  // Read Port A to send to keyboard read row
  uint8_t praOut = (m6526->regs[M6526_REG_PRA] | ~m6526->regs[M6526_REG_DDRA]);
  praOut &= joystick_getValue(&(m64_joysticks[1]));

  // read keyboard row
  uint8_t kbd = keyboard_readRow(praOut);

  // read the port 1 joystick
  uint8_t joy = joystick_getValue(&(m64_joysticks[0]));

  // pra is keyboard row value anded with joystick port 1
  return (kbd & joy);
}


void cia1_pulse(m6526_t *m6526) {

}

void cia1_init(uint32_t model) {
  // set up function pointers for cia1
  cia1.readPRA = &cia1_readPRA;
  cia1.readPRB = &cia1_readPRB;

  cia1.writePRA = &cia1_writePRA;
  cia1.writePRB = &cia1_writePRB;

  cia1.interrupt = &cia1_interrupt;
  cia1.pulse = &cia1_pulse;

  m6526_init(&cia1, model);
}
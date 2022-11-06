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


#ifndef M6526_H
#define M6526_H

#define M6526_MODEL_6526 0
#define M6526_MODEL_6526A 1

// 6526 registers
// cia1 starts at 0xdc00, cia2 starts at 0xdd00
#define M6526_REG_PRA         0x00     // data port A
#define M6526_REG_PRB         0x01     // data port B
#define M6526_REG_DDRA        0x02     // data direction reg A
#define M6526_REG_DDRB        0x03     // data direction reg B
#define M6526_REG_TIMERALO    0x04     // timer A low byte register 
#define M6526_REG_TIMERAHI    0x05     // timer A high byte register
#define M6526_REG_TIMERBLO    0x06     // timer B low byte register 
#define M6526_REG_TIMERBHI    0x07     // timer B high byte register
#define M6526_REG_TOD10TH     0x08     // 10ths of seconds register 
#define M6526_TOD_SEC       0x09     // seconds register 
#define M6526_REG_TODMIN      0x0a     // minutes register 
#define M6526_REG_TODHR       0x0b     // hours am/pm register 
#define M6526_REG_SDR         0x0c     // serial data register 
#define M6526_REG_ICR         0x0d     // interrupt control register 
#define M6526_REG_CRA         0x0e     // control register A 
#define M6526_REG_CRB         0x0f     // control register B 


// interrupt types
#define M6526_INTERRUPT_NONE         0           // no interrupt
#define M6526_INTERRUPT_UNDERFLOW_A  (1 << 0)    // timer a underflow
#define M6526_INTERRUPT_UNDERFLOW_B  (1 << 1)    // timer b underflow
#define M6526_INTERRUPT_ALARM        (1 << 2)    // tod alarm
#define M6526_INTERRUPT_SP           (1 << 3)    // serial port
#define M6526_INTERRUPT_FLAG         (1 << 4)    // external

typedef struct m6526_s m6526_t;

// function pointers, used for cia1/2 specific functions
typedef uint8_t (*m6526_read_function)(m6526_t *m6526);
typedef void (*m6526_write_function)(m6526_t *m6526, uint8_t data);
typedef void (*m6526_interrupt)(m6526_t *m6526, bool_t state);
typedef void (*m6526_pulse)(m6526_t *m6526);


struct m6526_s {
  int32_t model;

  event_t todEvent;

  // functions specific to cia1 and cia2
  m6526_read_function  readPRA;
  m6526_read_function  readPRB;
  m6526_write_function writePRA;
  m6526_write_function writePRB;
  m6526_interrupt      interrupt;
  m6526_pulse          pulse;

  timer_t timerA;
  timer_t timerB;

  // CIA registers (0xd_00 - 0xd_0f,   '_' is 'c' or 'd')
  uint8_t regs[0x10];

  // Serial Data Registers
  uint8_t  sdrOut;
  bool_t   sdrBuffered;
  uint32_t sdrCount;

  // used for 6526 timer b bug:
  uint64_t read_time;

  // Time Of Day
  bool_t todLatched;
  bool_t todStopped;

  uint8_t todClock[4];
  uint8_t todAlarm[4];
  uint8_t todLatch[4];

  int64_t todCycles;
  int64_t todPeriod;

  event_t interruptSourceEvent;
  bool_t scheduled;
  

  // The ICR provides masking (on write) and interrupt information (on read).

  // Interrupt Control Register $D_0D (Write)
  /*
    Write: (Bit 0..4 = INT MASK, Interrupt mask)
    Bit 0: 1 = Interrupt release through timer A underflow
    Bit 1: 1 = Interrupt release through timer B underflow
    Bit 2: 1 = Interrupt release if clock=alarmtime
    Bit 3: 1 = Interrupt release if a complete byte has been received/sent.
    Bit 4: 1 = Interrupt release if a positive slope occurs at the FLAG-Pin.
    Bit 5..6: unused
    Bit 7: Source bit. 0 = set bits 0..4 are clearing the according mask bit. 1 = set bits 0..4 are setting the according mask bit. If all bits 0..4 are cleared, there will be no change to the mask.
  */    
  uint8_t icrWrite;

  // Interrupt Control Register $D_0D (Read)
  /*
    Read: (Bit0..4 = INT DATA, Origin of the interrupt)
    Bit 0: 1 = Underflow Timer A
    Bit 1: 1 = Underflow Timer B
    Bit 2: 1 = Time of day and alarm time is equal
    Bit 3: 1 = SDR full or empty, so full byte was transferred, depending of operating mode serial bus
    Bit 4: 1 = IRQ Signal occured at FLAG-pin (cassette port Data input, serial bus SRQ IN)
    Bit 5..6: always 0
    Bit 7: 1 = IRQ An interrupt occured, so at least one bit of INT MASK and INT DATA is set in both registers.

    Flags will be cleared after reading the register
  */
  uint8_t icrRead;

};

void m6526_write(m6526_t *m6526, uint32_t addr, uint8_t data);
uint8_t m6526_read(m6526_t *m6526, uint32_t addr);

void m6526_init(m6526_t *m6526, uint32_t model);
void m6526_reset(m6526_t *m6526);

void m6526_setDayOfTimeRate(m6526_t *m6526, double clock);
void rescheduleToDEventFunction(void *context);
void todEventFunction(void *context);

// interrupt functions
void m6526_interrupt_event(void *context);
void m6526_interrupt_schedule(m6526_t *m6526);
void m6526_interrupt_init(m6526_t *m6526);
uint8_t m6526_interrupt_clear(m6526_t *m6526);
void m6526_interrupt_reset(m6526_t *m6526);
void m6526_interrupt_setEnabled(m6526_t *m6526, uint8_t interruptMask);
void m6526_interrupt_clearEnabled(m6526_t *m6526, uint8_t interruptMask);
void m6526_interrupt_trigger(m6526_t *m6526, uint8_t interruptMask);

#endif

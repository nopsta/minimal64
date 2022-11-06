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

void m6526_init(m6526_t *m6526, uint32_t model) {
  m6526->model = model;

  m6526->sdrOut = 0;
  m6526->sdrBuffered = false;
  m6526->sdrCount = 0;
  m6526->todLatched = false;
  m6526->todStopped = false;
  m6526->todCycles = 0;
  m6526->todPeriod = 0xffffffff;

  m6526_interrupt_init(m6526);

  timerA_init(&(m6526->timerA), m6526);
  timerB_init(&(m6526->timerB), m6526);

  m6526->todEvent.context = (void *)m6526;
  m6526->todEvent.event = &todEventFunction;

  m6526_reset(m6526);
}


void m6526_reset(m6526_t *m6526) {
  timer_reset(&(m6526->timerA));
  timer_reset(&(m6526->timerB));
  m6526->sdrOut = 0;
  m6526->sdrCount = 0;
  m6526->sdrBuffered = false;
  m6526_interrupt_reset(m6526);

  memset(m6526->regs, 0, 0x10);
  memset(m6526->todClock, 0, 4);
  memset(m6526->todAlarm, 0, 4);
  memset(m6526->todLatch, 0, 4);

  m6526->todLatched = false;
  m6526->todStopped = true;

  int32_t i;
  for(i = 0; i < 4; i++) {
    m6526->todAlarm[i] = 0;
    m6526->todClock[i] = 0;
  }
  // usually set to 1
  m6526->todClock[M6526_REG_TODHR - M6526_REG_TOD10TH] = 1;
  m6526->todCycles = 0;

  clock_scheduleEvent(&m64_clock, &(m6526->todEvent), 0, PHASE_PHI1);
}


// Read CIA register.
uint8_t m6526_read(m6526_t *m6526, uint32_t addr) {

  uint8_t data;
  
  addr &= 0xf;

  timer_syncWithCpu(&(m6526->timerA));
  timer_wakeUpAfterSyncWithCpu(&(m6526->timerA));

  timer_syncWithCpu(&(m6526->timerB));
  timer_wakeUpAfterSyncWithCpu(&(m6526->timerB));

  switch(addr) {
    case M6526_REG_PRA:  // 0xd_00
      // data port a
      // call the readPRA function for CIA1 or CIA2
      // and it with the bits in stored value for the register according to value of data direction a register
      data = (m6526->readPRA)(m6526) & (m6526->regs[M6526_REG_PRA] | ~m6526->regs[M6526_REG_DDRA]);
      return data;
    case M6526_REG_PRB:  // 0xd_01
      // data port b
      // call the readPRA function for CIA1 or CIA2
      // and it with the bits in stored value for the register according to value of data direction a register    
      data =  (m6526->readPRB)(m6526) & (m6526->regs[M6526_REG_PRB] | ~m6526->regs[M6526_REG_DDRB]);

      // pulse CIA 1 or 2
      (m6526->pulse)(m6526);

      // check bit 1 of Control Register A (0xd_0e)
      // if set to 1 then Port B bit 6 (PB6) contains output for Timer A

      if ((m6526->regs[M6526_REG_CRA] & 2) != 0) {
          // clear bit 6
          data &= 0xbf;
          // check bit 2 of Control Register A
          // 1 = Timer A Toggle
          // 0 = Timer A Pulse
          if ((m6526->regs[M6526_REG_CRA] & 0x04) != 0 ? timer_getPbToggle(&(m6526->timerA)) : (m6526->timerA.state & TIMER_CIAT_OUT) != 0) {
              data |= 0x40;
          }
      }

      // check bit 1 of Control Register B (0xd_0f)
      // if set to 1 then Port B bit 7 (PB7) contains output for Timer A
      if ((m6526->regs[M6526_REG_CRB] & 0x02) != 0) {
        // clear bit 7
        data &= 0x7f;

        // check bit 2 of Control Register B
        // 1 = Timer B Toggle
        // 0 = Timer B Pulse
        if ((m6526->regs[M6526_REG_CRB] & 4) != 0 ? timer_getPbToggle(&(m6526->timerB)) : (m6526->timerB.state & TIMER_CIAT_OUT) != 0) {
          data |= 0x80;
        }
      }
      return data;
    case M6526_REG_TIMERALO: // 0xd_04
      // Timer A Low Byte
      return (uint8_t)(timer_getTimer(&(m6526->timerA)) & 0xff) ;
    case M6526_REG_TIMERAHI: // 0xd_05
      // Timer A High Byte
      return (uint8_t)(timer_getTimer(&(m6526->timerA)) >> 8);
    case M6526_REG_TIMERBLO: // 0xd_06
      // Timer B Low Byte
      return (uint8_t)(timer_getTimer(&(m6526->timerB)) & 0xff);
    case M6526_REG_TIMERBHI: // 0xd_07
      // Timer B High Byte
      return (uint8_t)(timer_getTimer(&(m6526->timerB)) >> 8);
    case M6526_REG_TOD10TH:  // 0xd_08
    case M6526_TOD_SEC:    // 0xd_09
    case M6526_REG_TODMIN:   // 0xd_0a
    case M6526_REG_TODHR:    // 0xd_0b
      // Time Of Day Registers
      // see notes/04-time-of-day.txt

      // if not currently latched, copy time of day into the latch
      if (!m6526->todLatched) {
        memcpy(m6526->todLatch, m6526->todClock, sizeof(uint8_t) * 4);
      }

      if (addr == M6526_REG_TODHR) {
        // All four TOD registers latch on a read of Hours register
        m6526->todLatched = true;
      }

      if (addr == M6526_REG_TOD10TH) {
        // TOD registers become unlatched on read of 10ths of a second register
        m6526->todLatched = false;
      }

      // read the return data from the latch
      data = m6526->todLatch[addr - M6526_REG_TOD10TH];

      // set unused bits to 0 for each register
      if(addr == M6526_REG_TOD10TH) {
        return data & 0x0f;
      }
      if(addr == M6526_TOD_SEC || addr == M6526_REG_TODMIN) {
        return data & 0x7f;
      }
      if(addr == M6526_REG_TODHR) {
        return data & 0xbf;
      }
    case M6526_REG_ICR: // 0xd_0d
      // Interrupt Control Register

      // for old cia timer b bug: icr isnt set for timer b if one cycle after icr read
      // save the icr read time
      m6526->read_time = clock_getTime(&m64_clock, PHASE_PHI2);
      
      // flags are cleared after reading the register
      data = m6526_interrupt_clear(m6526);
      return data & 0xbf;
    case M6526_REG_CRA:  // 0xd_0e
      // CIA Control Register A
      return (uint8_t)(m6526->regs[M6526_REG_CRA] & 0xee) | (m6526->timerA.state & 1);
    case M6526_REG_CRB:  // 0xd_0f
      // CIA Control Register B    
      return (uint8_t)(m6526->regs[M6526_REG_CRB] & 0xee) | (m6526->timerB.state & 1);
    default:
      return m6526->regs[addr];
  }
}

// write CIA register
void m6526_write(m6526_t *m6526, uint32_t addr, uint8_t data) {
  // 16 registers (0-f)
  addr &= 0xf;

  // sync the timers before writing to registers
  timer_syncWithCpu(&(m6526->timerA));
  timer_syncWithCpu(&(m6526->timerB));  

  // store old value, used when setting control register A and B
  // to see if start bit going from 0 to 1
  uint8_t oldData = m6526->regs[addr];

  // store value in regs
  m6526->regs[addr] = data;

  switch (addr) {
    case M6526_REG_PRA:   // 0xd_00  Data Port A
    case M6526_REG_DDRA:  // 0xd_02  Data Direction Register Port A
        // call the CIA 1 or 2 specific function
        (m6526->writePRA)(m6526, (uint8_t)(m6526->regs[M6526_REG_PRA] | ~m6526->regs[M6526_REG_DDRA]) );
        break;
    case M6526_REG_PRB:   // 0xd_01  Data Port B
        (m6526->pulse)(m6526);
    case M6526_REG_DDRB:  // 0xd_02  Data Direction Register Port B
        (m6526->writePRB)(m6526, (uint8_t)(m6526->regs[M6526_REG_PRB] | ~m6526->regs[M6526_REG_DDRB]) );
        break;
    case M6526_REG_TIMERALO: // 0xd_04 
        // Timer A Low Byte 
        timer_setLatchLow(&(m6526->timerA), data);
        break;
    case M6526_REG_TIMERAHI: // 0xd_05
        // Timer A High Byte
        timer_setLatchHigh(&(m6526->timerA), data);
        break;
    case M6526_REG_TIMERBLO: // 0xd_06
        // Timer B Low Byte
        timer_setLatchLow(&(m6526->timerB), data);
        break;
    case M6526_REG_TIMERBHI: // 0xd_07
        // Timer B High Byte
        timer_setLatchHigh(&(m6526->timerB), data);
        break;
    case M6526_REG_TOD10TH: // 0xd_08
    case M6526_TOD_SEC:   // 0xd_09
    case M6526_REG_TODMIN:  // 0xd_0a
    case M6526_REG_TODHR:   // 0xd_0b
        // see notes/04-time-of-day.txt

        if(addr == M6526_REG_TOD10TH) {
          // bcd 0 - 9
          data &= 0x0f;
        } else if(addr == M6526_TOD_SEC || addr == M6526_REG_TODMIN ) {
          // bcd 00 - 59
          data &= 0x7f;
        } else if (addr == M6526_REG_TODHR) {
          // bcd 00 - 11, with am/pm on bit 7
          data &= 0x9f;
        }

        // check bit 7 of CIA Control Register B
        // 1 = writing to alarm, 0 = writing to clock
        if ((m6526->regs[M6526_REG_CRB] & 0x80) != 0) {
          // writing into alarm
            m6526->todAlarm[addr - M6526_REG_TOD10TH] = data;
            int32_t alarmslot = addr - M6526_REG_TOD10TH;
        } else {
          // writing into tod clock

          if (addr == M6526_REG_TODHR) {
            // when not writing to alarm, toggle am/pm (bit 7) on hour 12
            if ((data & 0x1f) == 0x12) {
              data ^= 0x80;
            }
          }

          // TOD is started on a write to the 10ths of a second register
          if (addr == M6526_REG_TOD10TH) {
            if(m6526->todStopped) {
              // if going from stopped to started, need to restart tod counter
              rescheduleToDEventFunction(m6526);
            }
            m6526->todStopped = false;
          }

          // TOD is stopped on a write to the Hours register
          if (addr == M6526_REG_TODHR) {
            m6526->todStopped = true;
          }

          // check if the register value has changed and then set the tod clock
          bool_t changed = m6526->todClock[addr - M6526_REG_TOD10TH] != data;
          m6526->todClock[addr - M6526_REG_TOD10TH] = data;

          // if changed and the new time of day equals the alarm time, trigger an interrupt
          // this only occurs if writing to the time of day, not when writing to the alarm
          if(changed && (memcmp(m6526->todAlarm, m6526->todClock, 4) == 0)) {
            m6526_interrupt_trigger(m6526, M6526_INTERRUPT_ALARM);
          }
        }

        break;
    case M6526_REG_SDR:  // 0xd_0c
        // serial IO data buffer
        // bit 6 of CIA Control Register A sets the Serial Port IO Mode
        // 1 = output, 0 = input
        if ((m6526->regs[M6526_REG_CRA] & 0x40) != 0) {
            // output mode
            m6526->sdrBuffered = true;
        }
        break;
    case M6526_REG_ICR:  // 0xd_0d
        // Interrupt Control Register
        /*
        Bit 0: 1 = Interrupt release through timer A underflow
        Bit 1: 1 = Interrupt release through timer B underflow
        Bit 2: 1 = Interrupt release if clock=alarmtime
        Bit 3: 1 = Interrupt release if a complete byte has been received/sent.
        Bit 4: 1 = Interrupt release if a positive slope occurs at the FLAG-Pin.
        Bit 5..6: unused
        Bit 7: Source bit. 0 = set bits 0..4 are clearing the according mask bit. 1 = set bits 0..4 are setting the according mask bit. 
        //If all bits 0..4 are cleared, there will be no change to the mask.
        */

        // Bit 7: Source bit. 0 = set bits 0..4 are clearing the according mask bit. 1 = set bits 0..4 are setting the according mask bit. 
        if ((data & 0x80) != 0) {
          // source bit is one
          m6526_interrupt_setEnabled(m6526, data);
        } else {
          // source bit is zero
          m6526_interrupt_clearEnabled(m6526, data);
        }
        break;
    case M6526_REG_CRA:   // 0x0e  // Control Timer A
    case M6526_REG_CRB:   // 0x0f  // Control Timer B
        {
            // get pointer to the timer for the register
            timer_t *t = (addr == M6526_REG_CRA) ? &(m6526->timerA) : &(m6526->timerB);

            // is start bit (bit 0) changing from 0 to 1
            bool_t start = (data & 1) != 0 && (oldData & 1) == 0;
            if (start) {
              timer_setPbToggle(t, true);
            }

            if (addr == M6526_REG_CRB) {
                timer_setControlRegister(t, (uint8_t)(data | (data & 0x40) >> 1));
            } else {
                timer_setControlRegister(t, data);
            }
            break;
        }
    default:
        break;
  }

  timer_wakeUpAfterSyncWithCpu(&(m6526->timerA));
  timer_wakeUpAfterSyncWithCpu(&(m6526->timerB));
}

// External interrupt control.
void m6526_setFlag(m6526_t *m6526, bool_t flag) {
  if (flag) {
    m6526_interrupt_trigger(m6526, M6526_INTERRUPT_FLAG);
  }
}
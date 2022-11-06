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

// event executed when timer A underflow
void timerA_bTick_event_function(void *context) {
  timer_t *timer = (timer_t *)context;
  m6526_t *m6526 = timer->m6526; 

  timer_syncWithCpu(&(m6526->timerB));
  m6526->timerB.state |= TIMER_CIAT_STEP;
  timer_wakeUpAfterSyncWithCpu(&(m6526->timerB));
}


void timerA_serialPort(timer_t *timer) {
  m6526_t *m6526 = timer->m6526; 

  if ((m6526->regs[M6526_REG_CRA] & 0x40) != 0) {
    if (m6526->sdrCount != 0) {
      if (--m6526->sdrCount == 0) {
        m6526_interrupt_trigger(m6526, M6526_INTERRUPT_SP);
      }
    }
    if (m6526->sdrCount == 0 && m6526->sdrBuffered) {
      m6526->sdrOut = m6526->regs[M6526_REG_SDR];
      m6526->sdrBuffered = false;
      m6526->sdrCount = 16;
    }
  }
};

void timerA_underFlow(timer_t *timer) {
  m6526_t *m6526 = timer->m6526; 

  // timer A underflow interrupt
  m6526_interrupt_trigger(m6526, M6526_INTERRUPT_UNDERFLOW_A);

  // signal the underflow to timer b
  if ((m6526->regs[M6526_REG_CRB] & 0x41) == 0x41) {
    if ((m6526->timerB.state & TIMER_CIAT_CR_START) != 0) {
      clock_scheduleEvent(&m64_clock, &(timer->bTick_event), 0, PHASE_PHI2);
    }
  }
}



void timerA_init(timer_t *timer, m6526_t *m6526) {
  timer->bTick_event.context = timer;
  timer->bTick_event.event = &timerA_bTick_event_function;

  timer->serialPort = &timerA_serialPort;
  timer->underFlow = &timerA_underFlow;

  timer->m6526 = m6526;
  
  timer_init(timer);
}

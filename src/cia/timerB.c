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


void timerB_serialPort(timer_t *timer) {
}


void timerB_underFlow(timer_t *timer) {
  m6526_t *m6526 = timer->m6526;

  if(m6526->model == M6526_MODEL_6526) {
    uint64_t time = clock_getTime(&m64_clock, PHASE_PHI2);
    if(time - 1 == m6526->read_time) {
      // timer b bug
      return;
    }
  }
  m6526_interrupt_trigger(timer->m6526, M6526_INTERRUPT_UNDERFLOW_B);
}

void timerB_init(timer_t *timer, m6526_t *m6526) {

  timer->serialPort = &timerB_serialPort;
  timer->underFlow = &timerB_underFlow;

  timer->m6526 = m6526;

  timer_init(timer);
}


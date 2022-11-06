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

// see notes/05-interrupt-control.txt

// functions to manage m6526 interrupts
#define INTERRUPTSOURCE_INTERRUPT_REQUEST ((1 << 7) | 0)

// used by old 6526 to call interrupt as delayed by one cycle
void m6526_interrupt_event(void *context) {
  m6526_t *m6526 = (m6526_t *)context;

  m6526->icrRead |= INTERRUPTSOURCE_INTERRUPT_REQUEST;

  (m6526->interrupt)(m6526, true);
  m6526->scheduled = false;
}

// need to schedule interrupts one cycle later to simulate bug
void m6526_interrupt_schedule(m6526_t *m6526) {
  if (!m6526->scheduled) {
    // delay to next cycle to simulate 6526 bug
    clock_scheduleEvent(&m64_clock, &(m6526->interruptSourceEvent), 1, PHASE_PHI1);
    m6526->scheduled = true;
  }
}

void m6526_interrupt_trigger(m6526_t *m6526, uint8_t interruptMask) {

  m6526->icrRead |= interruptMask;

  if ((m6526->icrWrite & m6526->icrRead) != 0 
    && (m6526->icrRead & INTERRUPTSOURCE_INTERRUPT_REQUEST) == 0) {

    // idr hasnt got interrupt request set, if it is set, means interrupt being serviced
    m6526_interrupt_schedule(m6526);
  }
}

// clear interrupt state and return flag
uint8_t m6526_interrupt_clear(m6526_t *m6526) {

  if (m6526->scheduled) {
    clock_cancelEvent(&m64_clock, &(m6526->interruptSourceEvent));
    m6526->scheduled = false;
  }

  if ((m6526->icrRead & INTERRUPTSOURCE_INTERRUPT_REQUEST) != 0) {
      (m6526->interrupt)(m6526, false);
  }

  uint8_t old = m6526->icrRead;
  m6526->icrRead = 0;
  return old;
}



void m6526_interrupt_reset(m6526_t *m6526) {
  m6526->icrWrite = m6526->icrRead = 0;
  clock_cancelEvent(&m64_clock, & (m6526->interruptSourceEvent));

  m6526->scheduled = false;
}


// set the interrupt control  bits with mask
void m6526_interrupt_setEnabled(m6526_t *m6526, uint8_t interruptMask) {
  m6526->icrWrite |= interruptMask & ~INTERRUPTSOURCE_INTERRUPT_REQUEST;

  m6526_interrupt_trigger(m6526, M6526_INTERRUPT_NONE);
}

// clear interrupt control bits with mask
void m6526_interrupt_clearEnabled(m6526_t *m6526, uint8_t interruptMask) {
  m6526->icrWrite &= ~interruptMask;
}


void m6526_interrupt_init(m6526_t  *m6526) {
  m6526->icrWrite = 0;
  m6526->icrRead = 0;
  m6526->interruptSourceEvent.context = (void *)m6526;
  m6526->interruptSourceEvent.event = &m6526_interrupt_event;
}

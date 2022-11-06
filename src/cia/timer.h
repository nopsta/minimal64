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


#ifndef TIMER_H
#define TIMER_H


#define TIMER_CIAT_CR_START 0x01 
#define TIMER_CIAT_STEP 0x04 
#define TIMER_CIAT_CR_ONESHOT 0x08 
#define TIMER_CIAT_CR_FLOAD 0x10 
#define TIMER_CIAT_PHI2IN 0x20 
#define TIMER_CIAT_CR_MASK (TIMER_CIAT_CR_START | TIMER_CIAT_CR_ONESHOT | TIMER_CIAT_CR_FLOAD | TIMER_CIAT_PHI2IN)

#define TIMER_CIAT_COUNT2 0x100 
#define TIMER_CIAT_COUNT3 0x200 

#define TIMER_CIAT_ONESHOT0 (0x08 << 8)
#define TIMER_CIAT_ONESHOT (0x08 << 16)
#define TIMER_CIAT_LOAD1 (0x10 << 8)
#define TIMER_CIAT_LOAD (0x10 << 16)

#define TIMER_CIAT_OUT 0x80000000 

typedef struct m6526_s m6526_t;
typedef struct timer_s timer_t;

typedef void (*timer_function)(timer_t *timer);

struct timer_s {
  event_t timer_event;
  event_t cycleSkippingEvent;
  event_t bTick_event;

  timer_function serialPort;
  timer_function underFlow;

  m6526_t *m6526;

  // CRA/CRB control register / state.
  int32_t state;

  // Copy of regs[CRA/B]
  uint8_t lastControlValue;

  // Current timer value
  uint16_t timer;

  // timer start value
  uint16_t latch;

  // PB6/PB7 Flipflop to signal underflows.
  bool_t pbToggle;

  /**
   * This is a tri-state:
   * 
   * when -1: cia is completely stopped when 0: cia 1-clock events are
   * ticking. otherwise: cycleskipevent is ticking, and the value is the
   * first phi1 clock of skipping.
   */
  int64_t ciaEventPauseTime;
};

typedef struct timer_s timer_t;

void timerA_init(timer_t *timer, m6526_t *m6526);
void timerB_init(timer_t *timer, m6526_t *m6526);

void timer_init(timer_t *timer);
void timer_reset(timer_t *timer);

void timer_clock(timer_t *timer) ;
void timer_reschedule(timer_t *timer);

int32_t timer_getTimer(timer_t *timer);
void timer_wakeUpAfterSyncWithCpu(timer_t *timer);
void timer_syncWithCpu(timer_t *timer);

void timer_setPbToggle(timer_t *timer, bool_t state);
bool_t timer_getPbToggle(timer_t *timer);
void timer_setControlRegister(timer_t *timer, uint8_t cr);

void timer_setLatchLow(timer_t *timer, uint8_t low);
void timer_setLatchHigh(timer_t *timer, uint8_t high);

#endif
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

void timer_eventFunction(void *context) {
  timer_t *timer = (timer_t *)context;

  timer_clock(timer);
  timer_reschedule(timer);
}


// get the timer back up to where it should be after skipping cycles
void timer_cycleSkippingEventFunction(void *context) {
  timer_t *timer = (timer_t *)context;

  int64_t elapsed = clock_getTime(&m64_clock, PHASE_PHI1) -timer->ciaEventPauseTime;
  timer->ciaEventPauseTime = 0;

  timer->timer -= elapsed;

  timer_eventFunction(context);
}

void timer_init(timer_t *timer) {

  timer->state = 0;
  timer->lastControlValue = 0;
  timer->timer = 0;
  timer->latch = 0;
  timer->pbToggle = 0;
  timer->ciaEventPauseTime = 0;

  timer->timer_event.context = (void *)timer;
  timer->timer_event.event = &timer_eventFunction;


  timer->cycleSkippingEvent.context = (void *)timer;
  timer->cycleSkippingEvent.event = &timer_cycleSkippingEventFunction;


}

// Set CRA/CRB control register.
void timer_setControlRegister(timer_t *timer, uint8_t cr) {
  timer->state &= ~TIMER_CIAT_CR_MASK;
  timer->state |= (cr & TIMER_CIAT_CR_MASK) ^ TIMER_CIAT_PHI2IN;
  timer->lastControlValue = cr;
}

// Get current timer value.
int32_t timer_getTimer(timer_t *timer) {
  return timer->timer;
}

// Get PB6/PB7 Flipflop state.
bool_t timer_getPbToggle(timer_t *timer) {
  return timer->pbToggle;
}

// Set PB6/PB7 Flipflop state.
// bit 6 and 7 of PRB
/*
Bit #6: RS232 CTS line; 1 = Sender is ready to send.
Bit #7: RS232 DSR line; 1 = Receiver is ready to receive.
*/
void timer_setPbToggle(timer_t *timer, bool_t state) {
  timer->pbToggle = state;
}


// Set high byte of Timer start value (Latch).
void timer_setLatchHigh(timer_t *timer, uint8_t high) {
  timer->latch = (uint16_t) (  (timer->latch & 0xff) | (high & 0xff) << 8);
  if ((timer->state & TIMER_CIAT_LOAD) != 0 || (timer->state & TIMER_CIAT_CR_START) == 0) {
    timer->timer = timer->latch;
  }
}

// Set low byte of Timer start value (Latch).
void timer_setLatchLow(timer_t *timer, uint8_t low) {
  timer->latch = (uint16_t) ( (timer->latch & 0xff00) | (low & 0xff));
  if ((timer->state & TIMER_CIAT_LOAD) != 0) {
    timer->timer = (uint16_t) ( (timer->timer & 0xff00) | (low & 0xff));
  }
}

void timer_reset(timer_t *timer) {
  clock_cancelEvent(&m64_clock, &(timer->timer_event));

  timer->timer = timer->latch = (uint16_t) 0xffff;
  timer->pbToggle = false;
  timer->state = 0;
  timer->ciaEventPauseTime = 0;

  clock_scheduleEvent(&m64_clock, &(timer->timer_event), 1, PHASE_PHI1);
}



// Perform cycle skipping manually.
// Clocks the CIA up to the state it should be in, and stops all events.
void timer_syncWithCpu(timer_t *timer) {
  if (timer->ciaEventPauseTime > 0) {

    clock_cancelEvent(&m64_clock, &(timer->cycleSkippingEvent));

    int64_t elapsed = clock_getTime(&m64_clock, PHASE_PHI2) - timer->ciaEventPauseTime;
    /*
     * It's possible for CIA to determine that it wants to go to
     * sleep starting from the next cycle, and then have its plans
     * aborted by CPU. Thus, we must avoid modifying the CIA state
     * if the first sleep clock was still in the future.
     */
    if (elapsed >= 0) {
      timer->timer -= elapsed;
      timer_clock(timer);
    }
  }
  if (timer->ciaEventPauseTime == 0) {
    clock_cancelEvent(&m64_clock, &(timer->timer_event));
  }
  timer->ciaEventPauseTime = -1;
}

// Counterpart of syncWithCpu(), starts the event ticking if it is
// needed. No clock() call or anything such is permissible here!
void timer_wakeUpAfterSyncWithCpu(timer_t *timer) {
  timer->ciaEventPauseTime = 0;
  clock_scheduleEvent(&m64_clock, &(timer->timer_event), 0, PHASE_PHI1);

}

// Execute one CIA state transition.
void timer_clock(timer_t *timer) {
  if (timer->timer != 0 && (timer->state & TIMER_CIAT_COUNT3) != 0) {
    timer->timer--;
  }

  /* ciatimer.c block start */
  uint32_t adj = timer->state & (TIMER_CIAT_CR_START | TIMER_CIAT_CR_ONESHOT | TIMER_CIAT_PHI2IN);
  if ((timer->state & (TIMER_CIAT_CR_START | TIMER_CIAT_PHI2IN)) == (TIMER_CIAT_CR_START | TIMER_CIAT_PHI2IN)) {
    adj |= TIMER_CIAT_COUNT2;
  }
  if ((timer->state & TIMER_CIAT_COUNT2) != 0 || (timer->state & (TIMER_CIAT_STEP | TIMER_CIAT_CR_START)) == (TIMER_CIAT_STEP | TIMER_CIAT_CR_START)) {
    adj |= TIMER_CIAT_COUNT3;
  }
  /*
   * CR_FLOAD -> LOAD1, CR_ONESHOT -> ONESHOT0, LOAD1 -> LOAD,
   * ONESHOT0 -> ONESHOT
   */
  adj |= (timer->state & (TIMER_CIAT_CR_FLOAD | TIMER_CIAT_CR_ONESHOT | TIMER_CIAT_LOAD1 | TIMER_CIAT_ONESHOT0)) << 8;
  timer->state = adj;
  /* ciatimer.c block end */

  if (timer->timer == 0 && (timer->state & TIMER_CIAT_COUNT3) != 0) {
    timer->state |= TIMER_CIAT_LOAD | TIMER_CIAT_OUT;

    if ((timer->state & (TIMER_CIAT_ONESHOT | TIMER_CIAT_ONESHOT0)) != 0) {
      timer->state &= ~(TIMER_CIAT_CR_START | TIMER_CIAT_COUNT2);
    }

    // By setting bits 2&3 of the control register,
    // PB6/PB7 will be toggled between high and low at each
    // underflow.
    bool_t toggle = (timer->lastControlValue & 0x06) == 6;
    timer->pbToggle = toggle && !timer->pbToggle;

    // Implementation of the serial port
    (timer->serialPort)(timer);

    // Timer A signals underflow handling: IRQ/B-count
    (timer->underFlow)(timer);
  }

  if ((timer->state & TIMER_CIAT_LOAD) != 0) {
    timer->timer = timer->latch;
    timer->state &= ~TIMER_CIAT_COUNT3;
  }
}

// schedule when to call the timer event again
// If timer is stopped or is programmed to just count down, the events are
// paused.
void timer_reschedule(timer_t *timer) {
  /*
   * There are only two subcases to consider.
   *
   * - are we counting, and if so, are we going to continue counting?
   * - have we stopped, and are there no conditions to force a new
   * beginning?
   * 
   * Additionally, there are numerous flags that are present only in
   * passing manner, but which we need to let cycle through the CIA
   * state machine.
   */
  uint32_t unwanted = TIMER_CIAT_OUT | TIMER_CIAT_CR_FLOAD | TIMER_CIAT_LOAD1 | TIMER_CIAT_LOAD;
  if ((timer->state & unwanted) != 0) {
    //context.schedule(this, 1);
    clock_scheduleEvent(&m64_clock, &(timer->timer_event), 1, -1);

    return;
  }

  if ((timer->state & TIMER_CIAT_COUNT3) != 0) {
    /*
     * Test the conditions that keep COUNT2 and thus COUNT3 alive,
     * and also ensure that all of them are set indicating steady
     * state operation.
     */

    uint32_t wanted = TIMER_CIAT_CR_START | TIMER_CIAT_PHI2IN | TIMER_CIAT_COUNT2 | TIMER_CIAT_COUNT3;
    if ((timer->timer & 0xffff) > 2 && (timer->state & wanted) == wanted) {
      /*
       * we executed this cycle, therefore the pauseTime is +1. If
       * we are called to execute on the very next clock, we need
       * to get 0 because there's another timer-- in it.
       */
      timer->ciaEventPauseTime = clock_getTime(&m64_clock, PHASE_PHI1) + 1;
      /* execute event slightly before the next underflow. */
      clock_scheduleEvent(&m64_clock, &(timer->cycleSkippingEvent), timer->timer - 1 & 0xffff, -1);

      return;
    }

    /* play safe, keep on ticking. */
    clock_scheduleEvent(&m64_clock, &(timer->timer_event), 1, -1);

    return;

  } else {
    /*
     * Test conditions that result in CIA activity in next clocks.
     * If none, stop.
     */
    uint32_t unwanted1 = TIMER_CIAT_CR_START | TIMER_CIAT_PHI2IN;
    uint32_t unwanted2 = TIMER_CIAT_CR_START | TIMER_CIAT_STEP;

    if ((timer->state & unwanted1) == unwanted1 || (timer->state & unwanted2) == unwanted2) {
      clock_scheduleEvent(&m64_clock, &(timer->timer_event), 1, -1);

      return;
    }

    timer->ciaEventPauseTime = -1;
    return;
  }
}

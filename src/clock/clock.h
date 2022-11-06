/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For the full
 * license text, see http://www.gnu.org/licenses/gpl.html.
 * 
 * Author: nopsta 2022
 * 
 * Based on the Event Scheduler from SIDPLAY2 which is based on alarm from VICE 
 * 
 */

#ifndef clock_H
#define clock_H

#define PHASE_PHI1 0
#define PHASE_PHI2 1

typedef void (*event_function)(void *context);

// the clock can be used to schedule events to run at certain cycles/phases
struct event {
  // trigger time in half cycles since clock was last reset
  uint64_t triggerTime;

  // pointer to the function to run at trigger time
  event_function event;

  // context is passed to the event function
  void *context;

  // next event in the linked list 
  struct event *next;
};
typedef struct event event_t;


struct clock_s {
  // current time is the total number of half cycles since last reset
  uint64_t clock_currentTime;

  double clock_cyclesPerSecond;

  // first event in linked list of events ordered by event trigger time
  event_t firstEvent;

  // if the clock reaches this event, there's nothing more to do
  event_t lastEvent;
};
typedef struct clock_s clock_t;


void clock_init(clock_t *clock, double cyclesPerSecond);
void clock_reset(clock_t *clock);

double clock_getCyclesPerSecond(clock_t *clock);

void clock_step(clock_t *clock);

uint64_t clock_getTime(clock_t *clock, uint32_t phase);
uint32_t clock_getPhase(clock_t *clock);
uint64_t clock_getTimeAndPhase(clock_t *clock);

void clock_scheduleEvent(clock_t *clock, event_t *event, uint32_t cycles, uint32_t phase);
void clock_runNextEvent(clock_t *clock);
void clock_cancelEvent(clock_t *clock, event_t *event);


#endif

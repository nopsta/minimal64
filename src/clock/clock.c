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


#include "../m64.h"

#include <stdio.h>
#include <string.h>

#define MAX_TIME 9223372036854775807
#define MIN_TIME -9223372036854775807


void firstEventFunction(void *context) {
  // nothing
}

void lastEventFunction(void *context) {

}

void clock_init(clock_t *clock, double cyclesPerSecond) {
  clock->clock_cyclesPerSecond = cyclesPerSecond;
  
  clock->firstEvent.triggerTime = MIN_TIME;
  clock->firstEvent.next = NULL;
  clock->firstEvent.context = NULL;
  clock->firstEvent.event = &firstEventFunction;

  clock->lastEvent.triggerTime = MIN_TIME;
  clock->lastEvent.next = NULL;
  clock->lastEvent.context = NULL;
  clock->lastEvent.event = &lastEventFunction;

  clock_reset(clock);
}

void clock_reset(clock_t *clock) {
  clock->clock_currentTime = 0;
  clock->firstEvent.next = &(clock->lastEvent);
}

// schedule an event relative to current time
void clock_scheduleEvent(clock_t *clock, event_t *event, uint32_t cycles, uint32_t phase) {
  if(phase != -1) {
    event->triggerTime = (cycles * 2) 
                         + clock->clock_currentTime 
                         + ( (clock->clock_currentTime & 1) ^ (phase == PHASE_PHI1 ? 0 : 1));
  } else {
    event->triggerTime = (cycles * 2) + clock->clock_currentTime;
  }

  // insert the event into linked list of events
  event_t *scan = &(clock->firstEvent);

  uint32_t count = 0;
  while(1) {
    event_t *next = scan->next;
    if(next->triggerTime > event->triggerTime) {
      event->next = next;
      scan->next = event;
      return;
    }

    scan = next;
    count++;
    if(count > 1000) {
      // give up
      break;
    }
  }
}


void clock_cancelEvent(clock_t *clock, event_t *event) {
  event_t *prev = &(clock->firstEvent);
  event_t *scan = prev->next;

  uint32_t count = 0;
  while ((scan->triggerTime <= event->triggerTime)) {
    if (event == scan) {
      prev->next = scan->next;
      return;
    }
    prev = scan;
    scan = scan->next;

    count++;
    if(count > 1000) {
      // give up
      break;
    }
  }
}



void clock_runNextEvent(clock_t *clock) {
  event_t *event = clock->firstEvent.next;
  if(event->next == NULL) {
    // uh oh, its the last event...
    return;
  }

  clock->firstEvent.next = event->next;

  clock->clock_currentTime = event->triggerTime;

  (event->event)(event->context);
}


void clock_step(clock_t *clock) {
  clock_runNextEvent(clock);

  uint64_t time = clock_getTimeAndPhase(clock);

  // execute all events with the same trigger time
  while( (clock->firstEvent.next)->triggerTime == time ) {
    clock_runNextEvent(clock);
  }
}

uint64_t clock_getTime(clock_t *clock, uint32_t phase) {
  return  (clock->clock_currentTime + (phase == PHASE_PHI1 ? 1 : 0)) / 2;
}


uint32_t clock_getPhase(clock_t *clock) {
  return (clock->clock_currentTime & 1) == 0 ? PHASE_PHI1 : PHASE_PHI2;
}

uint64_t clock_getTimeAndPhase(clock_t *clock) {
  return clock->clock_currentTime;
}

double clock_getCyclesPerSecond(clock_t *clock) {
  return clock->clock_cyclesPerSecond;
}

void clock_setCyclesPerSecond(clock_t *clock, double cyclesPerSecond) {
  clock->clock_cyclesPerSecond = cyclesPerSecond;
}

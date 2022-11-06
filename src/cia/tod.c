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

void todEventFunction(void *context) {
  m6526_t *m6526 = (m6526_t *)context;

  // Reload divider according to 50/60 Hz flag
  // Only performed on expiry according to Frodo
  // bit 7 of timer a control register is 50/60 hz flag
  if ((m6526->regs[M6526_REG_CRA] & 0x80) != 0) {
    m6526->todCycles += m6526->todPeriod * 5;
  } else {
    m6526->todCycles += m6526->todPeriod * 6;
  }

  // Fixed precision 25.7
  clock_scheduleEvent(&m64_clock, &(m6526->todEvent), m6526->todCycles >> 7, -1);

  m6526->todCycles &= 0x7f; // Just keep the decimal part

  if (!m6526->todStopped) {
    // increase the time of day by 1/10th sec

    // time is stored in bcd format, each decimal digit 0-9 takes 4 bits
    // extract the digits
    uint8_t tenth = m6526->todClock[0] & 0x0f;

    uint8_t secsDigit0 = m6526->todClock[1] & 0x0f;
    uint8_t secsDigit1 = (m6526->todClock[1] >> 4) & 0x0f;
    
    uint8_t minsDigit0 = m6526->todClock[2] & 0x0f;
    uint8_t minsDigit1 = (m6526->todClock[2] >> 4) & 0x0f;

    uint8_t hrsDigit0 = m6526->todClock[3] & 0x0f;
    uint8_t hrsDigit1 = (m6526->todClock[3] >> 4) & 0x01;
    uint8_t amPm =  m6526->todClock[3] & 0x80;

    // increase tenths of sec
    tenth = (tenth + 1) & 0x0f;
    if (tenth == 10) {
      // tenths of sec has reached 10, so increase secs
      tenth = 0;
      secsDigit0 = (secsDigit0 + 1) & 0x0f;
      if (secsDigit0 == 10) {
        // first digit of secs has reached 10, increase second digit
        secsDigit0 = 0;
        secsDigit1 = (secsDigit1 + 1) & 0x07;
        if (secsDigit1 == 6) {
          // second secs digit reached 6, increase minutes
          secsDigit1 = 0;
          minsDigit0 = (minsDigit0 + 1) & 0x0f;
          if (minsDigit0 == 10) {
            // first mins digit reached 10, inc the second digit
            minsDigit0 = 0;
            minsDigit1 = (minsDigit1 + 1) & 0x07;
            if (minsDigit1 == 6) {
              // second mins digit reached 6, inc hrs
              minsDigit1 = 0;
              hrsDigit0 = (hrsDigit0 + 1) & 0x0f;
              if (hrsDigit1) {
                // hrs is 1x
                if (hrsDigit0 == 2) {
                  // hrs gone from 11->12 so toggle am/pm
                  amPm ^= 0x80;
                }

                if (hrsDigit0 == 3) {
                  // hrs has gone from 12->13, reset it to 01
                  hrsDigit0 = 1;
                  hrsDigit1 = 0;
                }
              } else {
                if (hrsDigit0 == 10) {
                  hrsDigit0 = 0;
                  hrsDigit1 = 1;
                }
              }
            }
          }
        }
      }
    }

    // save the updated values
    m6526->todClock[0] = tenth;
    m6526->todClock[1] = secsDigit0 | (secsDigit1 << 4);
    m6526->todClock[2] = minsDigit0 | (minsDigit1 << 4);
    m6526->todClock[3] = hrsDigit0 | (hrsDigit1 << 4) | amPm;

    // check alarm
    if (memcmp(m6526->todAlarm, m6526->todClock, 4) == 0)  {      
      m6526_interrupt_trigger(m6526, M6526_INTERRUPT_ALARM);
    }
  }
}

// tod counter supposed to restart when clock goes from stopped to started
void rescheduleToDEventFunction(void *context) {
  m6526_t *m6526 = (m6526_t *)context;

  // The TOD clock's internal circuitry is designed to be driven by either 50 or 60 Hz clock signal, 
  // which can be inexpensively derived from the mains power source AC
  // Only performed on expiry according to Frodo
  // bit 7 of timer a control register is 50/60 hz flag
  if ((m6526->regs[M6526_REG_CRA] & 0x80) != 0) {
    m6526->todCycles += m6526->todPeriod * 5;
  } else {
    m6526->todCycles += m6526->todPeriod * 6;
  }

  clock_cancelEvent(&m64_clock, &(m6526->todEvent));
  // Fixed precision 25.7
  clock_scheduleEvent(&m64_clock, &(m6526->todEvent), m6526->todCycles >> 7, -1);
}


// set day of time rate
void m6526_setDayOfTimeRate(m6526_t *m6526, double clock) {
  m6526->todPeriod = (int64_t)(clock * (1 << 7));
}


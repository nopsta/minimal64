/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For the full
 * license text, see http://www.gnu.org/licenses/gpl.html.
 */

#ifndef JOYSTICK_H
#define JOYSTICK_H

#define JOYSTICK_UP    0x1
#define JOYSTICK_DOWN  0x2
#define JOYSTICK_LEFT  0x4
#define JOYSTICK_RIGHT 0x8
#define JOYSTICK_FIRE  0x10


struct joystick {
  uint8_t value;
};

typedef struct joystick joystick_t;

void joystick_reset(joystick_t *joystick);
uint8_t joystick_getValue(joystick_t *joystick);
void joystick_push(joystick_t *joystick, uint8_t direction);
void joystick_release(joystick_t *joystick, uint8_t direction);


#endif
/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For the full
 * license text, see http://www.gnu.org/licenses/gpl.html.
 */

#include "../m64.h"

joystick_t m64_joysticks[2];

void joystick_reset(joystick_t *joystick) {
  joystick->value = 0xff;
}

uint8_t joystick_getValue(joystick_t *joystick) {
  return joystick->value;
}

void m64_joystickPush(uint32_t joystick, uint32_t direction) {
  m64_joysticks[joystick].value = m64_joysticks[joystick].value | direction;
  m64_joysticks[joystick].value = m64_joysticks[joystick].value ^ direction;
}

void m64_joystickRelease(uint32_t joystick, uint32_t direction) {
  m64_joysticks[joystick].value = m64_joysticks[joystick].value | direction;
}

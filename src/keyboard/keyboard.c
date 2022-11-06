/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For the full
 * license text, see http://www.gnu.org/licenses/gpl.html.
 */

#include "../m64.h"

key_t keyboard_keys[65];
bool_t keyboard_keyDown[65];

void keyboard_reset() {
  uint32_t i;

  for(i = 0; i < 65; i++) {
    keyboard_keyDown[i] = false;
  }
}

void keyboard_init() {
  keyboard_keys[KEY_ARROW_LEFT].row = 7;
  keyboard_keys[KEY_ARROW_LEFT].col = 1;

  keyboard_keys[KEY_ONE].row = 7;
  keyboard_keys[KEY_ONE].col = 0;

  keyboard_keys[KEY_TWO].row = 7;
  keyboard_keys[KEY_TWO].col = 3;

  keyboard_keys[KEY_THREE].row = 1;
  keyboard_keys[KEY_THREE].col = 0;
  
  keyboard_keys[KEY_FOUR].row = 1;
  keyboard_keys[KEY_FOUR].col = 3;

  keyboard_keys[KEY_FIVE].row = 2;
  keyboard_keys[KEY_FIVE].col = 0;

  keyboard_keys[KEY_SIX].row = 2;
  keyboard_keys[KEY_SIX].col = 3;

  keyboard_keys[KEY_SEVEN].row = 3;
  keyboard_keys[KEY_SEVEN].col = 0;

  keyboard_keys[KEY_EIGHT].row = 3;
  keyboard_keys[KEY_EIGHT].col = 3;

  keyboard_keys[KEY_NINE].row = 4;
  keyboard_keys[KEY_NINE].col = 0;

  keyboard_keys[KEY_ZERO].row = 4;
  keyboard_keys[KEY_ZERO].col = 3;

  keyboard_keys[KEY_PLUS].row = 5;
  keyboard_keys[KEY_PLUS].col = 0;

  keyboard_keys[KEY_MINUS].row = 5;
  keyboard_keys[KEY_MINUS].col = 3;

  keyboard_keys[KEY_POUND].row = 6;
  keyboard_keys[KEY_POUND].col = 0;

  keyboard_keys[KEY_CLEAR_HOME].row = 6;
  keyboard_keys[KEY_CLEAR_HOME].col = 3;

  keyboard_keys[KEY_INS_DEL].row = 0;
  keyboard_keys[KEY_INS_DEL].col = 0;

  keyboard_keys[KEY_CTRL].row = 7;
  keyboard_keys[KEY_CTRL].col = 2;

  keyboard_keys[KEY_Q].row = 7;
  keyboard_keys[KEY_Q].col = 6;

  keyboard_keys[KEY_W].row = 1;
  keyboard_keys[KEY_W].col = 1;

  keyboard_keys[KEY_E].row = 1;
  keyboard_keys[KEY_E].col = 6;

  keyboard_keys[KEY_R].row = 2;
  keyboard_keys[KEY_R].col = 1;

  keyboard_keys[KEY_T].row = 2;
  keyboard_keys[KEY_T].col = 6;

  keyboard_keys[KEY_Y].row = 3;
  keyboard_keys[KEY_Y].col = 1;

  keyboard_keys[KEY_U].row = 3;
  keyboard_keys[KEY_U].col = 6;

  keyboard_keys[KEY_I].row = 4;
  keyboard_keys[KEY_I].col = 1;

  keyboard_keys[KEY_O].row = 4;
  keyboard_keys[KEY_O].col = 6;

  keyboard_keys[KEY_P].row = 5;
  keyboard_keys[KEY_P].col = 1;

  keyboard_keys[KEY_AT].row = 5;
  keyboard_keys[KEY_AT].col = 6;

  keyboard_keys[KEY_STAR].row = 6;
  keyboard_keys[KEY_STAR].col = 1;

  keyboard_keys[KEY_ARROW_UP].row = 6;
  keyboard_keys[KEY_ARROW_UP].col = 6;

  keyboard_keys[KEY_RUN_STOP].row = 7;
  keyboard_keys[KEY_RUN_STOP].col = 7;

  keyboard_keys[KEY_A].row = 1;
  keyboard_keys[KEY_A].col = 2;

  keyboard_keys[KEY_S].row = 1;
  keyboard_keys[KEY_S].col = 5;

  keyboard_keys[KEY_D].row = 2;
  keyboard_keys[KEY_D].col = 2;

  keyboard_keys[KEY_F].row = 2;
  keyboard_keys[KEY_F].col = 5;

  keyboard_keys[KEY_G].row = 3;
  keyboard_keys[KEY_G].col = 2;

  keyboard_keys[KEY_H].row = 3;
  keyboard_keys[KEY_H].col = 5;

  keyboard_keys[KEY_J].row = 4;
  keyboard_keys[KEY_J].col = 2;

  keyboard_keys[KEY_K].row = 4;
  keyboard_keys[KEY_K].col = 5;

  keyboard_keys[KEY_L].row = 5;
  keyboard_keys[KEY_L].col = 2;

  keyboard_keys[KEY_COLON].row = 5;
  keyboard_keys[KEY_COLON].col = 5;

  keyboard_keys[KEY_SEMICOLON].row = 6;
  keyboard_keys[KEY_SEMICOLON].col = 2;

  keyboard_keys[KEY_EQUALS].row = 6;
  keyboard_keys[KEY_EQUALS].col = 5;

  keyboard_keys[KEY_RETURN].row = 0;
  keyboard_keys[KEY_RETURN].col = 1;

  keyboard_keys[KEY_COMMODORE].row = 7;
  keyboard_keys[KEY_COMMODORE].col = 5;

  keyboard_keys[KEY_SHIFT_LEFT].row = 1;
  keyboard_keys[KEY_SHIFT_LEFT].col = 7;

  keyboard_keys[KEY_Z].row = 1;
  keyboard_keys[KEY_Z].col = 4;

  keyboard_keys[KEY_X].row = 2;
  keyboard_keys[KEY_X].col = 7;

  keyboard_keys[KEY_C].row = 2;
  keyboard_keys[KEY_C].col = 4;

  keyboard_keys[KEY_V].row = 3;
  keyboard_keys[KEY_V].col = 7;

  keyboard_keys[KEY_B].row = 3;
  keyboard_keys[KEY_B].col = 4;

  keyboard_keys[KEY_N].row = 4;
  keyboard_keys[KEY_N].col = 7;

  keyboard_keys[KEY_M].row = 4;
  keyboard_keys[KEY_M].col = 4;

  keyboard_keys[KEY_COMMA].row = 5;
  keyboard_keys[KEY_COMMA].col = 7;

  keyboard_keys[KEY_PERIOD].row = 5;
  keyboard_keys[KEY_PERIOD].col = 4;

  keyboard_keys[KEY_SLASH].row = 6;
  keyboard_keys[KEY_SLASH].col = 7;

  keyboard_keys[KEY_SHIFT_RIGHT].row = 6;
  keyboard_keys[KEY_SHIFT_RIGHT].col = 4;

  keyboard_keys[KEY_CURSOR_UP_DOWN].row = 0;
  keyboard_keys[KEY_CURSOR_UP_DOWN].col = 7;

  keyboard_keys[KEY_CURSOR_LEFT_RIGHT].row = 0;
  keyboard_keys[KEY_CURSOR_LEFT_RIGHT].col = 2;

  keyboard_keys[KEY_SPACE].row = 7;
  keyboard_keys[KEY_SPACE].col = 4;

  keyboard_keys[KEY_F1].row = 0;
  keyboard_keys[KEY_F1].col = 4;

  keyboard_keys[KEY_F3].row = 0;
  keyboard_keys[KEY_F3].col = 5;

  keyboard_keys[KEY_F5].row = 0;
  keyboard_keys[KEY_F5].col = 6;

  keyboard_keys[KEY_F7].row = 0;
  keyboard_keys[KEY_F7].col = 3;

  keyboard_keys[KEY_RESTORE].row = -1;
  keyboard_keys[KEY_RESTORE].col = -1;

  keyboard_reset();

}

void m64_keyPush(uint32_t key) {
  keyboard_keyDown[key] = true;
}

void m64_keyRelease(uint32_t key) {
  keyboard_keyDown[key] = false;
}

uint8_t keyboard_readMatrix(uint8_t selected, bool_t wantRow) {
  uint8_t result = 0xff;
  uint32_t i;
  for(i = 0; i < 65; i++) {
    if(keyboard_keyDown[i]) {
      if(wantRow) {
        if((selected & 1 << keyboard_keys[i].col) == 0) {
          result &= ~(1 << keyboard_keys[i].row);
        }
      } else {
        if((selected & 1 << keyboard_keys[i].row) == 0) {
          result &= ~(1 << keyboard_keys[i].col);
        }

      }

    }
  }
  return result;
}

uint8_t keyboard_readColumn(uint8_t selected) {
  return keyboard_readMatrix(selected, true);
}

uint8_t keyboard_readRow(uint8_t selected) {
  return keyboard_readMatrix(selected, false);
}
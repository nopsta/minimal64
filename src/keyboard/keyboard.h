/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For the full
 * license text, see http://www.gnu.org/licenses/gpl.html.
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEY_ARROW_LEFT 0
#define KEY_ONE 1
#define KEY_TWO 2
#define KEY_THREE 3
#define KEY_FOUR 4
#define KEY_FIVE 5
#define KEY_SIX 6
#define KEY_SEVEN 7
#define KEY_EIGHT 8
#define KEY_NINE 9
#define KEY_ZERO 10
#define KEY_PLUS 11
#define KEY_MINUS 12
#define KEY_POUND 13
#define KEY_CLEAR_HOME 14
#define KEY_INS_DEL 15
#define KEY_CTRL 16
#define KEY_Q 17
#define KEY_W 18
#define KEY_E 19
#define KEY_R 20
#define KEY_T 21
#define KEY_Y 22
#define KEY_U 23
#define KEY_I 24
#define KEY_O 25
#define KEY_P 26
#define KEY_AT 27
#define KEY_STAR 28
#define KEY_ARROW_UP 29
#define KEY_RUN_STOP 30
#define KEY_A 31
#define KEY_S 32
#define KEY_D 33
#define KEY_F 34
#define KEY_G 35
#define KEY_H 36
#define KEY_J 37
#define KEY_K 38
#define KEY_L 39
#define KEY_COLON 40
#define KEY_SEMICOLON 41
#define KEY_EQUALS 42
#define KEY_RETURN 43
#define KEY_COMMODORE 44
#define KEY_SHIFT_LEFT 45
#define KEY_Z 46
#define KEY_X 47
#define KEY_C 48
#define KEY_V 49
#define KEY_B 50
#define KEY_N 51
#define KEY_M 52
#define KEY_COMMA 53
#define KEY_PERIOD 54
#define KEY_SLASH 55
#define KEY_SHIFT_RIGHT 56
#define KEY_CURSOR_UP_DOWN 57
#define KEY_CURSOR_LEFT_RIGHT 58
#define KEY_SPACE 59
#define KEY_F1 60
#define KEY_F3 61
#define KEY_F5 62
#define KEY_F7 63
#define KEY_RESTORE 64


struct key {
  uint32_t row;
  uint32_t col;
};

typedef struct key key_t;

void m64_keyPush(uint32_t key);
void m64_keyRelease(uint32_t key);

void keyboard_init();
void keyboard_reset();
uint8_t keyboard_readMatrix(uint8_t selected, bool_t wantRow);
uint8_t keyboard_readColumn(uint8_t selected);
uint8_t keyboard_readRow(uint8_t selected);

#endif
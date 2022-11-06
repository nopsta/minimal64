#ifndef M64_H
#define M64_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef uint8_t bool_t;
#define false 0
#define true 1

#define M64_MODEL_NTSC  0
#define M64_MODEL_PAL   1

#include "clock/clock.h"
#include "cpu/m6510.h"
#include "memory/banks.h"
#include "memory/pla.h"
#include "vic/sprite.h"
#include "vic/vic.h"
#include "vic/sprite.h"
#include "cartridge/cartridge.h"

#include "iec/iecBus.h"
#include "joystick/joystick.h"
#include "keyboard/keyboard.h"

#include "cia/timer.h"
#include "cia/m6526.h"
#include "cia/cia.h"

#include "sid/sid.h"

extern m6510_t m64_cpu;
extern clock_t m64_clock;
extern joystick_t m64_joysticks[2];
extern cartridge_t m64_cartridge;

void m64_init(int32_t model, int32_t sidModel);

void m64_injectAndRunPrg(uint8_t *data, uint32_t dataLength, uint32_t delay);
void m64_injectPrg(uint8_t *data, uint32_t dataLength);
void m64_loadCartridge(uint8_t *data, uint32_t dataLength);
unsigned char *m64_getPixelBuffer();
int32_t m64_update(int32_t deltaTime);

void m64_reset(uint32_t runUntilKernalIsReady);


#endif
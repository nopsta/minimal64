/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For the full
 * license text, see http://www.gnu.org/licenses/gpl.html.
 * 
 * Author: nopsta 2022
 * 
 * Based VIC.java from Jsidplay2, 
 * authors: Jörg Jahnke (joergjahnke@users.sourceforge.net)
 *          Antti S. Lankila (alankila@bel.fi)
 * 
 * see vic/notes for more on the VICII
 */
#ifndef VIC_H
#define VIC_H

extern uint32_t quadrupleBits[16];

#define VIC_MAX_WIDTH (48 * 8)
#define VIC_MAX_HEIGHT 312

/*
source: http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
          | Video  | # of  | Visible | Cycles/ |  Visible
   Type   | system | lines |  lines  |  line   | pixels/line
 ---------+--------+-------+---------+---------+------------
 6567R56A | NTSC-M |  262  |   234   |   64    |    411
  6567R8  | NTSC-M |  263  |   235   |   65    |    418
   6569   |  PAL-B |  312  |   284   |   63    |    403

          | First  |  Last  |              |   First    |   Last
          | vblank | vblank | First X coo. |  visible   |  visible
   Type   |  line  |  line  |  of a line   |   X coo.   |   X coo.
 ---------+--------+--------+--------------+------------+-----------
 6567R56A |   13   |   40   |  412 ($19c)  | 488 ($1e8) | 388 ($184)
  6567R8  |   13   |   40   |  412 ($19c)  | 489 ($1e9) | 396 ($18c)
   6569   |  300   |   15   |  404 ($194)  | 480 ($1e0) | 380 ($17c)   
*/

// 6567R8
#define VIC_MODEL6567R8 0
#define M6567R8_CYCLES_PER_LINE    65
#define M6567R8_NUMBER_OF_LINES    263
#define M6567R8_FIRST_DISPLAY_LINE 25 //40 - set to 25 to make borders equal
#define M6567R8_LAST_DISPLAY_LINE  13


#define VIC_MODEL6569 1
#define M6569_CYCLES_PER_LINE    63
#define M6569_NUMBER_OF_LINES    312
#define M6569_FIRST_DISPLAY_LINE 15
#define M6569_LAST_DISPLAY_LINE  300


// just make it big enough to cover both chips
#define VIC_PIXELS_LENGTH (48 * 8 * 312)


/*
from http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
 A Bad Line Condition is given at any arbitrary clock cycle, if at the
 negative edge of ø0 at the beginning of the cycle RASTER >= $30 and RASTER
 <= $f7 and the lower three bits of RASTER are equal to YSCROLL and if the
 DEN bit was set during an arbitrary cycle of raster line $30.
*/
#define VIC_FIRST_DMA_LINE 0x30   
#define VIC_LAST_DMA_LINE  0xf7  

#define VIC_SPRITECOUNT 8

#define VIC_M656X_INTERRUPT_LP  (1 << 3)

// registers
#define VIC_SPRITE0X                 0x0
#define VIC_SPRITE0Y                 0x1
#define VIC_SPRITE1X                 0x2
#define VIC_SPRITE1Y                 0x3
#define VIC_SPRITE2X                 0x4
#define VIC_SPRITE2Y                 0x5
#define VIC_SPRITE3X                 0x6
#define VIC_SPRITE3Y                 0x7
#define VIC_SPRITE4X                 0x8
#define VIC_SPRITE4Y                 0x9
#define VIC_SPRITE5X                 0xa
#define VIC_SPRITE5Y                 0xb
#define VIC_SPRITE6X                 0xc
#define VIC_SPRITE6Y                 0xd
#define VIC_SPRITE7X                 0xe
#define VIC_SPRITE7Y                 0xf
#define VIC_SPRITEXMSB               0x10
#define VIC_CONTROL1                 0x11
#define VIC_RASTERCOUNTER            0x12
#define VIC_LIGHTPENX                0x13
#define VIC_LIGHTPENY                0x14
#define VIC_SPRITEENABLED            0x15
#define VIC_CONTROL2                 0x16
#define VIC_SPRITEEXPY               0x17
#define VIC_MEMORYSETUP              0x18
#define VIC_INTERRUPT                0x19
#define VIC_INTERRUPTENABLED         0x1a
#define VIC_SPRITEDATAPRIORITY       0x1b
#define VIC_SPRITEMULTICOLOR         0x1c
#define VIC_SPRITEEXPX               0x1d
#define VIC_SPRITESPRITECOLLISION    0x1e
#define VIC_SPRITEDATACOLLISION      0x1f
#define VIC_BORDERCOLOR              0x20
#define VIC_BGCOLOR0                 0x21
#define VIC_BGCOLOR1                 0x22
#define VIC_BGCOLOR2                 0x23
#define VIC_BGCOLOR3                 0x24
#define VIC_SPRITEMULTICOLOR0        0x25
#define VIC_SPRITEMULTICOLOR1        0x26
#define VIC_SPRITECOLOR0             0x27
#define VIC_SPRITECOLOR1             0x28
#define VIC_SPRITECOLOR2             0x29
#define VIC_SPRITECOLOR3             0x2a
#define VIC_SPRITECOLOR4             0x2b
#define VIC_SPRITECOLOR5             0x2c
#define VIC_SPRITECOLOR6             0x2d
#define VIC_SPRITECOLOR7             0x2e

extern int32_t vic_model;
extern uint8_t vic_videoModeColors[10];
extern sprite_t vic_sprites[8];

extern uint8_t vic_registers[0x40];

extern int32_t vic_charMemBase;
extern int32_t vic_bitmapMemBase;
extern int32_t vic_videoMatrixBase;

extern uint8_t vic_colorData[40];
extern uint8_t vic_videoMatrixData[40];

extern int32_t vic_rasterY;
extern int32_t vic_CYCLES_PER_LINE;

extern int32_t vic_vc;
extern int32_t vic_vcBase;
extern int32_t vic_rc;
extern bool_t vic_isBadLine;
extern bool_t vic_isDisplayActive;
extern bool_t vic_startOfFrame;
extern bool_t vic_areBadLinesEnabled ;
extern int32_t vic_MAX_RASTERS;
extern bool_t vic_lpAsserted;

extern uint8_t vic_latchedColor;
extern uint8_t vic_latchedVmd;
extern uint32_t vic_oldGraphicsData;

extern int32_t vic_CYCLES_PER_LINE;
extern int32_t vic_MAX_RASTERS;

extern int32_t vic_yscroll;
extern int8_t vic_xscroll;

extern int32_t vic_latchedXscroll;


extern bool_t vic_graphicsRendering;

extern event_t vic_rasterYIRQEdgeDetector;

extern bool_t vic_isDisplayActive;
extern bool_t vic_rasterYIRQCondition;
extern bool_t vic_showBorderVertical;
extern bool_t vic_showBorderMain;

extern uint32_t vic_nextPixel;
extern int32_t vic_cycle;

extern uint8_t vic_phi1Data;


extern uint8_t vic_lpx;
extern uint8_t vic_lpy;
extern bool_t vic_lpTriggered;

extern uint32_t vic_pixels[VIC_PIXELS_LENGTH];
extern uint32_t vic_pixelBuffer[VIC_PIXELS_LENGTH];

extern uint32_t vic_colors[16];
extern uint32_t vic_borderColor;
extern uint32_t vic_pixelColor;
extern bool_t vic_mcFlip;
extern uint32_t vic_phi1DataPipe;

extern uint8_t vic_irqFlags;
extern uint8_t vic_irqMask;

extern event_t m6569_event;
extern event_t m6567_event;


void vic_init(int32_t model);
void vic_reset();

void vic_write(uint16_t reg, uint8_t data);
uint8_t vic_read(uint16_t reg);

uint8_t vic_getLastReadByte();
void vic_interrupt(bool_t b);
void vic_activateIRQFlag(uint8_t flag);

void vic_interrupt(bool_t b);
void vic_setBA(bool_t b);
uint8_t vic_vicReadColorMemoryPHI2(uint16_t address);
uint8_t vic_vicReadMemoryPHI1(uint16_t address);
uint8_t vic_vicReadMemoryPHI2(uint16_t address);
void vic_handleSpriteVisibilityEvent(sprite_t *sprite);
void vic_setSpriteDataFromCPU(uint8_t data);
int32_t vic_getCurrentSpriteCycle();
int32_t vic_readSpriteXCoordinate(int32_t spriteIndex);
void vic_triggerLightpen();
void vic_clearLightpen();
void vic_drawSpritesAndGraphics();
void vic_spriteCollisionsOnly();
void vic_fetchSpriteData(int32_t n);
void vic_fetchSpritePointer(int32_t n);
bool_t vic_readDEN();
void vic_doVideoMatrixAccess();
bool_t vic_readRSEL();
bool_t vic_readDEN();
void vic_lightpenEdgeDetector();
bool_t vic_evaluateIsBadLine();

void m6569_init();
void m6569_reset();
void m6569_stop();
void m6569_start();

void m6567_init();
void m6567_reset();
void m6567_stop();
void m6567_start();


#endif
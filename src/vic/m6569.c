/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For the full
 * license text, see http://www.gnu.org/licenses/gpl.html.
 * 
 * Author: nopsta 2022
 * 
 * Based MOS6569.java from Jsidplay2, author: Antti Lankila 
 * 
 * see vic/notes for more on the VICII
 */

#include "../m64.h"

#include <stdio.h>

event_t m6569_event;
bool_t m6569_running = true;


// for cycle timing info, see notes/m6569-pal-timing.txt
void m6569_doPHI1Fetch() {
  int32_t n;
  int32_t address;
  int32_t offset;

  // 63 cycles per line
  // x coord = 0 at cycle 13 phi 2

  switch (vic_cycle) {
    case 56:
    case 57:
      // idle 2 cycles before sprite pointer fetches (56, 57)
      // In idle state, only g-accesses (char generator or bitmap) occur. The access is always to address
      // $3fff ($39ff when the ECM bit in register $d016 is set). -- need to add ecm bit check?
      vic_phi1Data = vic_vicReadMemoryPHI1(0x3fff);
      return;
    case 58:
    case 60:
    case 62:
    case 1:
    case 3:
    case 5:
    case 7:
    case 9:
    {
      // sprite pointer access

      // get the sprite index
      n = ((vic_cycle + 5) % vic_CYCLES_PER_LINE) >> 1;

      // sprite pointers are 0x3f8 bytes after video matrix base
      vic_phi1Data = vic_vicReadMemoryPHI1(vic_videoMatrixBase | 0x03f8 | n);
      return;
    }
    case 59:
    case 61:
    case 63:
    case 2:
    case 4:
    case 6:
    case 8:
    case 10:
      // if sprite is enabled, read sprite data, otherwise do idle

      // get the sprite index
      n = ((vic_cycle + 4) % vic_CYCLES_PER_LINE) >> 1;

      if (sprite_isDMA(&(vic_sprites[n]))) {
        // sprite active
        address = sprite_getCurrentByteAddress(&(vic_sprites[n]));
        vic_phi1Data = vic_vicReadMemoryPHI1(address);
      } else {
        // do idle
        //In idle state, only g-accesses (char generator or bitmap) occur. The access is always to address 0x3fff
        vic_phi1Data = vic_vicReadMemoryPHI1(0x3fff);
      }
      return;
    default: // (cycles 16-55)
      address = 0x3fff;

      if ((vic_registers[0x11] & 0x40) != 0) {
        // bit 6 of d011 is ecm
        address ^= 0x600;
      }

      if (vic_isDisplayActive) {
        // get the address for the chargen/bitmap access (g-access)

        // check VIC Control Register for bitmap mode
        if ((vic_registers[0x11] & 0x20) != 0) {
          // bitmap mode
          address &= vic_bitmapMemBase | vic_vc << 3 | vic_rc;
        } else {
          // one of the character modes

          // get the column in video matrix (0-39)
          n = vic_cycle - 16;

          // get the address of the character at the column
          address &= vic_charMemBase | ((vic_videoMatrixData[n] & 0xff) << 3) | vic_rc;
        }

        // VC and VMLI are incremented after each g-access in display state.
        // normally the VC counts all 1000 addresses of the video
        // matrix within the display frame and that RC counts the 8 pixel lines of
        // each text line.
        vic_vc = vic_vc + 1 & 0x3ff;
      }

      vic_phi1Data = vic_vicReadMemoryPHI1(address);
      return;

    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
      // from http://www.unusedino.de/ec64/technical/misc/vic656x/vic656x.html
      // The VIC does five read accesses in every raster line for the refresh of the
      // dynamic RAM. An 8 bit refresh counter (REF) is used to generate 256 DRAM
      // row addresses. The counter is reset to $ff in raster line 0 and decremented
      // by 1 after each refresh access.
      // So the VIC will access addresses $3fff, $3ffe, $3ffd, $3ffc and $3ffb in
      // line 0, addresses $3ffa, $3ff9, $3ff8, $3ff7 and $3ff6 in line 1 etc.

      // dram refresh.
      // offset counts backwards on each cycle
      offset =  (0xff - vic_rasterY * 5 - (vic_cycle - 11) ) &  0xff;
      vic_phi1Data = vic_vicReadMemoryPHI1(0x3f00 | offset);
  }
}



// execute the m6569 per cycle
void m6569_cycle(void *context) {
  sprite_t *sprite;
  int32_t i;
  int32_t narrowing;

  // on PAL, vic cycle goes from 1 - 63
  vic_cycle++;
  if(vic_cycle > vic_CYCLES_PER_LINE) {
    vic_cycle = 1;
  }

  // vic_graphicsRendering is set to true when vic_rasterY = M6569_FIRST_DISPLAY_LINE
  // and set to false when vic_rasterY = M6569_LAST_DISPLAY_LINE
  if (vic_graphicsRendering && (vic_cycle >= 14 && vic_cycle < 62) )  {
    vic_drawSpritesAndGraphics();
  } else {
    vic_spriteCollisionsOnly();
  }

  // do the phi 1 accesses for vic
  m6569_doPHI1Fetch();

  switch (vic_cycle) {
    /*
      In the first phases of cycle 55 and 56, the VIC checks for every sprite
      if the corresponding MxE bit in register $d015 is set and the Y
      coordinate of the sprite (odd registers $d001-$d00f) match the lower 8
      bits of RASTER. If this is the case and the DMA for the sprite is still
      off, the DMA is switched on, MCBASE is cleared, and if the MxYE bit is
      set the expansion flip flop is reset.
    */
    case 55:
      for (i = 0; i < VIC_SPRITECOUNT; i++) {
        sprite = &(vic_sprites[i]);
        if (sprite_isEnabled(sprite) && sprite_getY(sprite) == (vic_rasterY & 0xff)) {
          sprite_beginDMA(sprite);
        }
      }
      vic_setBA(!sprite_isDMA(&(vic_sprites[0])));
      break;
    case 56:
      // work out which sprites are enabled
      for (i = 0; i < VIC_SPRITECOUNT; i++) {
        sprite = &(vic_sprites[i]);
        if (sprite_isEnabled(sprite) && sprite_getY(sprite) == (vic_rasterY & 0xff)) {
          sprite_beginDMA(sprite);
          // if dma is switched on, then set allow display of sprite to be set in cycle 58
          sprite_setAllowDisplay(sprite, true);
        } else {
          sprite_setAllowDisplay(sprite, false);
        }
        sprite_expandYFlipFlop(sprite);
      }
      vic_setBA(!sprite_isDMA(&(vic_sprites[0]) ));
      break;
    case 57:
      // need to take cycle from cpu to read sprite data if it is enabled
      vic_setBA(!sprite_isDMA(&(vic_sprites[0]) ) && !sprite_isDMA(&(vic_sprites[1]) ));
      break;
    case 58:
      /*
        http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
        In the first phase of cycle 58, the VIC checks if RC=7. If so, the video
        logic goes to idle state and VCBASE is loaded from VC (VC->VCBASE). If
        the video logic is in display state afterwards (this is always the case
        if there is a Bad Line Condition), RC is incremented.
      */
      if (vic_rc == 7) {
          vic_vcBase = vic_vc;
          vic_isDisplayActive = vic_isBadLine;
      }
      if (vic_isDisplayActive) {
        // vic_rc is a 3 bit counter (0-7)
        vic_rc = (vic_rc + 1) & 0x7;
      }

      /*
        http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
        In the first phase of cycle 58, the MC of every sprite is loaded from
        its belonging MCBASE (MCBASE->MC) and it is checked if the DMA for the
        sprite is turned on and the Y coordinate of the sprite matches the lower
        8 bits of RASTER. If this is the case, the display of the sprite is
        turned on.
      */

      for (i = 0; i < VIC_SPRITECOUNT; i++) {
        sprite = &(vic_sprites[i]);
        if (sprite_isEnabled(sprite) && sprite_getY(sprite) == (vic_rasterY & 0xff)) {
          sprite_setDisplay(sprite, true);
        }
        if (!sprite_isDMA(sprite)) {
          sprite_setDisplay(sprite, false);
        }

        // initdmaaccess will set mc equal to mcbase
        sprite_initDmaAccess(sprite);
      }

      // set the sprite pointer to the value in vic_phi1data
      // read the first byte of line data for sprite, increment mc
      vic_fetchSpritePointer(0);
      break;
    case 59:
      // read sprite data, take bus away from cpu if sprite is active
      vic_fetchSpriteData(0);
      vic_setBA(!sprite_isDMA(&(vic_sprites[0])) && !sprite_isDMA(&(vic_sprites[1])) && !sprite_isDMA(&(vic_sprites[2])));
      break;
    case 60:
      /* sprite 1 pointer access */
      vic_fetchSpritePointer(1);
      vic_setBA(!sprite_isDMA(&(vic_sprites[1])) && !sprite_isDMA(&(vic_sprites[2])));
      break;
    case 61:
      vic_fetchSpriteData(1);
      vic_setBA(!sprite_isDMA(&(vic_sprites[1])) && !sprite_isDMA(&(vic_sprites[2])) && !sprite_isDMA(&(vic_sprites[3])));
      break;
    case 62:
      vic_fetchSpritePointer(2);
      vic_setBA(!sprite_isDMA(&(vic_sprites[2])) && !sprite_isDMA(&(vic_sprites[3])));
      break;
    case 63:
      vic_fetchSpriteData(2);
      vic_setBA(!sprite_isDMA(&(vic_sprites[2])) && !sprite_isDMA(&(vic_sprites[3])) && !sprite_isDMA(&(vic_sprites[4])));
      break;
    case 1:
    {
      // cycle 1
      // first cycle of line, so need to setup stuff

      // increase rasterY if not on the last line
      // if it's the last line, set raster y to zero on next cycle as last line is 1 cycle longer
      if (vic_rasterY == vic_MAX_RASTERS - 1) {
        // Once somewhere outside of the range of raster lines $30-$f7 (i.e.
        // outside of the Bad Line range), VCBASE is reset to zero. This is
        // presumably done in raster line 0
        vic_vcBase = 0;

        // set rasterY to zero on next cycle to account for last line being 1 cycle longer
        vic_startOfFrame = true;
      } else {
        vic_rasterY++;

        // check if need to fire an interrupt
        vic_rasterYIRQEdgeDetector.event(NULL);
      }

      // if it's the first line where badlines are possible, set if badlines are enabled
      if (vic_rasterY == VIC_FIRST_DMA_LINE) {
        // if display is enabled, then bad lines will be enabled
        vic_areBadLinesEnabled = vic_readDEN();
      }

      // is this line a badline?
      vic_isBadLine = vic_evaluateIsBadLine();
      vic_isDisplayActive = vic_isDisplayActive || vic_isBadLine;

      // 24 or 25 lines of text?
      // border will be delayed by 4 lines if 24 lines of text is enabled
      narrowing = vic_readRSEL() ? 0 : 4;

      // reached end of top border?
      if (vic_rasterY == (VIC_FIRST_DMA_LINE + 3 + narrowing) && vic_readDEN()) {
        vic_showBorderVertical = false;
      }

      // reached start of bottom border?
      // to open top and bottom borders, rsel (row select) is modified so this
      // comparison is never true
      if (vic_rasterY == VIC_LAST_DMA_LINE + 4 - narrowing) {
        vic_showBorderVertical = true;
      }

      vic_latchedXscroll = vic_xscroll << 2;

      // reset old graphics data
      vic_oldGraphicsData = 0;


      if (vic_rasterY == M6569_FIRST_DISPLAY_LINE) {
        vic_graphicsRendering = true;
        vic_nextPixel = 0;
      }

      if (vic_rasterY == M6569_LAST_DISPLAY_LINE + 1) {
        vic_graphicsRendering = false;
      }
      vic_fetchSpritePointer(3);
      vic_setBA(!sprite_isDMA(&(vic_sprites[3])) && !sprite_isDMA(&(vic_sprites[4])));
      break;
    }

    case 2:
    {
      // setting raster Y to 0 happens one cycle later than the usual incrementing raster y

      if (vic_startOfFrame) {
        vic_startOfFrame = false;
        vic_rasterY = 0;

        // check if need to trigger an interrupt
        vic_rasterYIRQEdgeDetector.event(NULL);

        // light pen
        vic_lpTriggered = false;
        vic_lightpenEdgeDetector();
      }
      vic_setBA(!sprite_isDMA(&(vic_sprites[3])) && !sprite_isDMA(&(vic_sprites[4])) && !sprite_isDMA(&(vic_sprites[5])));
      vic_fetchSpriteData(3);

      break;
    }

    case 3:
      vic_fetchSpritePointer(4);
      vic_setBA(!sprite_isDMA(&(vic_sprites[4])) && !sprite_isDMA(&(vic_sprites[5])));
      break;

    case 4:
      vic_fetchSpriteData(4);
      vic_setBA(!sprite_isDMA(&(vic_sprites[4])) && !sprite_isDMA(&(vic_sprites[5])) && !sprite_isDMA(&(vic_sprites[6])));
      break;
    case 5:
      vic_fetchSpritePointer(5);
      vic_setBA(!sprite_isDMA(&(vic_sprites[5])) && !sprite_isDMA(&(vic_sprites[6])));
      break;
    case 6:
      vic_fetchSpriteData(5);
      vic_setBA(!sprite_isDMA(&(vic_sprites[5])) && !sprite_isDMA(&(vic_sprites[6])) && !sprite_isDMA(&(vic_sprites[7])));
      break;
    case 7:
      vic_fetchSpritePointer(6);
      vic_setBA(!sprite_isDMA(&(vic_sprites[6])) && !sprite_isDMA(&(vic_sprites[7])));
      break;
    case 8:
      vic_fetchSpriteData(6);
      break;
    case 9:
      vic_fetchSpritePointer(7);
      vic_setBA(!sprite_isDMA(&(vic_sprites[7])));
      break;
    case 10:
      vic_fetchSpriteData(7);
      break;
    case 11:
      // give cpu back control of bus
      vic_setBA(true);
      break;
    case 12:
      // vic has control of bus from here if it's a bad line
      vic_setBA(!vic_isBadLine);
      break;
    case 13: 
      break;
    case 14:
      // In the first phase of cycle 14 of each line, VC is loaded from VCBASE
      // (VCBASE->VC) and VMLI is cleared. If there is a Bad Line Condition in
      // this phase, RC (row counter) is also reset to zero.

      vic_vc = vic_vcBase;
      if (vic_isBadLine) {
        vic_rc = 0;
      }
      break;
    case 15:
      // if bad line vic is reading data
      if (vic_isBadLine) {
        vic_doVideoMatrixAccess();
      }
      break;
    case 16:

      if (vic_isBadLine) {
        // its a bad line so read video matrix and colour ram
        vic_doVideoMatrixAccess();
      }

      // sprite dma ends on this cycle
      for (i = 0; i < VIC_SPRITECOUNT; i++) {
        sprite = &(vic_sprites[i]);
        if (sprite_isDMA(sprite)) {
            sprite_finishDmaAccess(sprite);
        }
      }
      break;

    default:
      /* graphics memory access */
      if (vic_isBadLine) {
        vic_doVideoMatrixAccess();
      }
      break;
  }

  if(m6569_running) {
    // schedule to run again
    clock_scheduleEvent(&m64_clock, &m6569_event, 1, -1);
  }
};


void m6569_init() {
  m6569_event.event = &m6569_cycle;
}

void m6569_reset() {
  // set to end, first call to m6569_cycle will increment and then set to 1
  vic_cycle = vic_CYCLES_PER_LINE;
  m6569_start();
}

void m6569_start() {
  m6569_running = true;
  clock_scheduleEvent(&m64_clock, &m6569_event, 0, PHASE_PHI1);
}

void m6569_stop() {
  m6569_running = false;
  clock_cancelEvent(&m64_clock, &m6569_event);
}
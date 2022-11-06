/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For the full
 * license text, see http://www.gnu.org/licenses/gpl.html.
 * 
 * Author: nopsta 2022
 * 
 * Based M6567.java from Jsidplay2, 
 * authors: Jörg Jahnke (joergjahnke@users.sourceforge.net)
 *          Antti S. Lankila (alankila@bel.fi)
 * 
 * see vic/notes for more on the VICII
 */

#include "../m64.h"

void sprite_init(sprite_t *sprite, sprite_t *linkedListHead, uint32_t index) {

  sprite->index = 0;
  sprite->display = false;
  sprite->consuming = false;
  sprite->firstMultiColorRead = false;

  sprite->offsetPixels = 0;

  sprite->pointerByte = 0;

  sprite->lineData = 0;
  sprite->consumedLineData = 0;

  sprite->mc = 0;
  sprite->mcBase = 0;

  sprite->firstYRead = false;
  sprite->firstXRead = false;
  sprite->x = 0;
  sprite->y = 0;
  sprite->enabled = false;
  sprite->expandX = false;
  sprite->expandY = false;
  sprite->multiColor = false;
  sprite->priorityOverForegroundGraphics = false;
  sprite->priorityMask = 0;
  sprite->multiColorLatched = false;
  sprite->prevPixel = 0;
  sprite->expandXLatched = false;
  sprite->allowDisplay = false;
  sprite->prevPriority = 0;
  sprite->colorBuffer = 0;

  sprite->indexBits = (8 | index) * 0x11111111;
  sprite->linkedListHead = linkedListHead;
  sprite->index = index;


  sprite->event.event = &sprite_event;
  sprite->event.context = sprite;
}


void sprite_event(void *context) {
  sprite_t *sprite = (sprite_t *)context;

  if (sprite->consuming) {
    // sprite is already being displayed
    return;
  }

  /* no display bit? ignore event. */
  if (!sprite->display) {
    return;
  }

  sprite->consuming = true;
  uint32_t delayLength = sprite->offsetPixels;

  if (sprite->expandX) {
    sprite->firstXRead = (delayLength & 1) == 0;
    delayLength >>= 1;
  } else {
    sprite->firstXRead = true;
  }

  if (sprite->multiColor) {
    sprite->firstMultiColorRead = (delayLength & 1) == 0;
  } else {
    sprite->firstMultiColorRead = true;
  }


  // read 24 bit sprite line data into top 24 bits of consumed line data, shifted by delay length 
  sprite->consumedLineData = sprite->lineData << (8 - delayLength);
  sprite->lineData = 0;

  sprite->prevPixel = 0;
  sprite->prevPriority = 0;
  sprite->expandXLatched = sprite->expandX;
  sprite->multiColorLatched = sprite->multiColor;
  sprite->priorityMask = sprite->priorityOverForegroundGraphics ? 0xffffffff : 0;

  /* put ourselves in appropriate place in the sprite linked list */
  sprite_t *current = sprite->linkedListHead;
  /* we are already being rendered? Don't reschedule, or it will jam. */
  while (true) {
    /* find a place to tuck ourselves in */
    if (current->nextVisibleSprite == NULL || current->nextVisibleSprite->index < sprite->index) {
      sprite->nextVisibleSprite = current->nextVisibleSprite;
      current->nextVisibleSprite = sprite;
      break;
    }

    current = current->nextVisibleSprite;
  }
}



// delay pixels is 0-7
void sprite_setDisplayStart(sprite_t *sprite, uint32_t offsetPixels) {
  sprite->offsetPixels = offsetPixels;
}

// get the x coordinate of the sprite
int32_t sprite_getX(sprite_t *sprite) {
  return sprite->x;
}

// set the x coordinate of the sprite
void sprite_setX(sprite_t *sprite, int32_t x) {
  sprite->x = x;
}

// get the y coordinate of the sprite
int32_t sprite_getY(sprite_t *sprite) {
  return sprite->y;
}


// set the y coordinate of the sprite
void sprite_setY(sprite_t *sprite, int32_t y) {
  sprite->y = y;
}

// Set whether the sprite has priority over the screen background
void sprite_setPriorityOverForegroundGraphics(sprite_t *sprite, bool_t priority) {
  if (priority == sprite->priorityOverForegroundGraphics) {
    return;
  }

  sprite->priorityOverForegroundGraphics = priority;

  if (sprite->priorityOverForegroundGraphics) {
    sprite->priorityMask = 0xff000000;
  } else {
    sprite->priorityMask = 0x00ffffff;
  }
}

bool_t sprite_isEnabled(sprite_t *sprite) {
  return sprite->enabled;
}

void sprite_setEnabled(sprite_t *sprite, bool_t enabled) {
  sprite->enabled = enabled;
}

void sprite_setExpandX(sprite_t *sprite, bool_t expandX) {
  sprite->expandX = expandX;
}

void sprite_setExpandY(sprite_t *sprite, bool_t expandY, bool_t crunchCycle) {
  if (sprite->expandY == expandY) {
    return;
  }
  sprite->expandY = expandY;

  // clearing y expansion bit resets the flip-flop
  if (!expandY) {
    sprite->firstYRead = false;
    // line crunch. 
    if (crunchCycle) {
      sprite->mc = (42 & sprite->mc & sprite->mcBase) | (21 & (sprite->mc | sprite->mcBase));
    }
  }
}

void sprite_setMulticolor(sprite_t *sprite, bool_t multiColor) {
  sprite->multiColor = multiColor;
}

void sprite_beginDMA(sprite_t *sprite) {
  if (sprite_isDMA(sprite)) {
    return;
  }
  sprite->mcBase = 0;
  sprite->firstYRead = false;
}

bool_t sprite_isDMA(sprite_t *sprite) {
  /*
    http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
    In the first phase of cycle 16, it is checked if the expansion flip flop
    is set. If so, MCBASE is incremented by 1. After that, the VIC checks if
    MCBASE is equal to 63 and turns of the DMA and the display of the sprite
    if it is.  
  */
  return sprite->mcBase != 63;
}


void sprite_initDmaAccess(sprite_t *sprite) {
  /*
    In the first phase of cycle 58, the MC of every sprite is loaded from
    its belonging MCBASE (MCBASE->MC)  
  */
  sprite->mc = sprite->mcBase;
}

/** Increment sprite read pointer during Y expansion */
void sprite_finishDmaAccess(sprite_t *sprite) {
  // we have to read this line again if the Y-expansion is set and this
  // was the first read
  if (!sprite->firstYRead) {
    sprite->mcBase = sprite->mc;
  }
}


void sprite_setDisplay(sprite_t *sprite, bool_t display) {
  if (display && !sprite->allowDisplay) {
    return;
  }
  sprite->display = display;
}

void sprite_setPointerByte(sprite_t *sprite, uint8_t pointerByte) {
  sprite->pointerByte = pointerByte;
}

// get the address of the next byte for the sprite to access
uint32_t sprite_getCurrentByteAddress(sprite_t *sprite) {
  //  three sprite data accesses per line, sprite->mc is incremented after each access
  uint32_t pointer = (sprite->pointerByte & 0xff) << 6;
  uint32_t address = pointer | sprite->mc;
  sprite->mc = (sprite->mc + 1) & 0x3f;
  return address;
}

// set a byte in the 24 bit sprite shift register
void sprite_setLineByte(sprite_t *sprite, uint32_t byteIndex, uint8_t value) {
  // clear the byte
  sprite->lineData &= ~(0xff << (byteIndex * 8));
  // set the byte
  sprite->lineData |= (value & 0xff) << (byteIndex * 8);
}


// toggle expand y
void sprite_expandYFlipFlop(sprite_t *sprite) {
  if (sprite->expandY) {
    sprite->firstYRead = !sprite->firstYRead;
  }
}

void sprite_setColor(sprite_t *sprite, uint32_t idx, uint32_t val) {
  sprite->color[idx] = val;
}


// fill the color buffer for the next 8 pixels
// return the mask for those pixels
// color buffer has 4 bits per color (colors 0-15)
// mask has 4 bits per pixel (pixel bits are quadrupled)
uint32_t sprite_calculateNext8Pixels(sprite_t *sprite) {
  uint32_t priorityBuffer = 0;

  // unexpanded sprite
  if (!sprite->expandX && !sprite->expandXLatched && !sprite->multiColor && !sprite->multiColorLatched) {

    // quadrupleBits takes a 4 bit number and quadruples the bits to form a 16 bit number
    // eg 1011 -> 1111000011111111

    // get 4 pixels and quadruple the bits, makes a 16 bit number
    uint32_t maskHigh = quadrupleBits[sprite->consumedLineData >> 28];
    sprite->consumedLineData <<= 4;

    // get next 4 pixels and quadruple the bits, makes a 16 bit number
    uint32_t maskLow = quadrupleBits[sprite->consumedLineData >> 28];
    sprite->consumedLineData <<= 4;

    if (sprite->consumedLineData == 0) {
      sprite->consuming = false;
    }
    uint32_t mask = maskHigh << 16 | (maskLow & 0xffff);

    // color[2] is the sprite color
    // color buffer stores color for 8 pixels, each pixel is 4 bits
    sprite->colorBuffer = (sprite->color[2] * 0x11111111) & mask;
    return mask;
  }

  sprite->colorBuffer = 0;

  /*
    * sprite state:
    * 
    * 1) multicolor 4-state: on, off, turning on, turning off 2) expand-x
    * 4-state: on, off, turning on, turning off 3) expandx current flop 4)
    * multicolor current flop
    * 
    * yielding 64 different decoding combinations + first pixel handling
    * for the flop-related cases.
    */

  uint32_t pixelIndex;
  for (pixelIndex = 0; pixelIndex < 8; pixelIndex++) {
    switch (pixelIndex) {
      case 6:
        if (sprite->expandXLatched != sprite->expandX) {
          sprite->expandXLatched = sprite->expandX;
          if (!sprite->expandXLatched) {
            sprite->firstXRead = true;
          }
        }
        break;
      case 7:
        if (sprite->multiColorLatched != sprite->multiColor) {
          sprite->multiColorLatched = sprite->multiColor;
          sprite->firstMultiColorRead = false;
        }
        break;
    }

    if (sprite->firstXRead) {
      if (sprite->multiColorLatched) {
        if (sprite->firstMultiColorRead) {
          if (sprite->consumedLineData == 0) {
            sprite->consuming = false;
          }

          sprite->prevPriority = (sprite->consumedLineData >> 30) != 0 ? 0xf : 0;
          sprite->prevPixel = sprite->color[sprite->consumedLineData >> 30];
        }
        sprite->firstMultiColorRead = !sprite->firstMultiColorRead;
      } else {
        if (sprite->consumedLineData == 0) {
          sprite->consuming = false;
        }

        sprite->prevPriority = 0;
        if(sprite->consumedLineData & 0x80000000) {
          sprite->prevPriority = 0xf;
        }
        sprite->prevPixel = sprite->prevPriority & sprite->color[2];
      }
      sprite->consumedLineData <<= 1;
    }
    if (sprite->expandXLatched) {
      sprite->firstXRead = !sprite->firstXRead;
    }

    // each entry in the colour buffer is 4 bits
    // shift everything up by 4 bits and stick new 4 bits at the start
    sprite->colorBuffer <<= 4;
    sprite->colorBuffer |= sprite->prevPixel;

    priorityBuffer <<= 4;
    priorityBuffer |= sprite->prevPriority;
  }

  return priorityBuffer;
}

/**
 * Damage the sprite display around the pointer fetch region.
 * 
 * The 3rd pixel into that region is duplicated 9 times, and nothing after
 * that is shown.
 */
void sprite_repeatPixels(sprite_t *sprite) {
  sprite->consumedLineData &= 0xc0000000;
  if ((sprite->consumedLineData & 0x40000000) != 0) {
    sprite->consumedLineData |= 0x7f800000;
  }
}

uint32_t sprite_getNextPriorityMask(sprite_t *sprite) {
  uint32_t mask = sprite->priorityMask;
  sprite->priorityMask = (mask >> 24) * 0x01010101;

  return mask;
}

void sprite_setAllowDisplay(sprite_t *sprite, bool_t allowDisplay) {
  sprite->allowDisplay = allowDisplay;
}
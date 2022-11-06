/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For the full
 * license text, see http://www.gnu.org/licenses/gpl.html.
 * 
 * Author: nopsta 2022
 * 
 * Based VIC.java from Jsidplay2, 
 * 
 * see vic/notes for more on the VICII
 */


#include "../m64.h"


// interrupt types
#define VIC_IRQ_RASTER 1 
#define VIC_IRQ_SPRITE_BACKGROUND_COLLISION 2 
#define VIC_IRQ_SPRITE_SPRITE_COLLISION 4 


// vic registers
uint8_t vic_registers[0x40];

int32_t vic_model = VIC_MODEL6569;

// VIDEO MODES - See notes/graphic-modes.txt
// Video Mode colour types
// background colour
#define VIC_COL_D021    0 
// extra background colour 1
#define VIC_COL_D022    1 
// extra background colour 2
#define VIC_COL_D023    2 
// colour from the colour buffer
#define VIC_COL_CBUF    4 
// mulicolour value from the colour buffer
#define VIC_COL_CBUF_MC 5 

// low 4 bits from video matrix, used in bitmap modes
#define VIC_COL_VBUF_L  6 
// high 4 bits from video matrix, used in bitmap modes
#define VIC_COL_VBUF_H  7 

// ecm mode has 64 characters
// bits 7 and 8 in video matrix are used to determine which bg color to use (d021, d022, d023, d024)
#define VIC_COL_ECM     8 
#define VIC_COL_NONE    9 

// The Video mode color decoder takes a 3-bit color mode offset and a 2-bit color value and returns the color mode
// the color mode is then used to find the current color value (0-15) for that mode from vic_videoModeColors
// color mode offset is determined from d011 (bit 5 and 6) and d016 registers (bit 4)
//  bit 0: color bit 0
//  bit 1: color bit 1
//  bit 2: 1 = multicolor mode  (offset bit 0, bit 5 of d011)
//  bit 3: 1 = bitmap mode      (offset bit 1, bit 6 of d011)
//  bit 4: 1 = extended background mode (offset bit 2, bit 4 of d016)

int8_t vic_videoModeColorDecoder[32] = {
          //  ECM=0 BMM=0 MCM=0
          // Standard Character Mode (only care about color bit 1)
          VIC_COL_D021,     // %000 00
          VIC_COL_D021,     // %000 01 
          VIC_COL_CBUF,     // %000 10 
          VIC_COL_CBUF,     // %000 11 

          // ECM=0 BMM=0 MCM=1
          // Multicolor Mode
          VIC_COL_D021,     // %001 00 
          VIC_COL_D022,     // %001 01 
          VIC_COL_D023,     // %001 10 
          VIC_COL_CBUF_MC,  // %001 11

          // ECM=0 BMM=1 MCM=0 
          // Standard Bitmap Mode (only care about color bit 1)
          VIC_COL_VBUF_L,   // %010 00
          VIC_COL_VBUF_L,   // %010 01
          VIC_COL_VBUF_H,   // %010 10
          VIC_COL_VBUF_H,   // %010 11

          // ECM=0 BMM=1 MCM=1
          // Multicolor Bitmap Mode
          VIC_COL_D021,     // %011 00
          VIC_COL_VBUF_H,   // %011 01
          VIC_COL_VBUF_L,   // %011 10
          VIC_COL_CBUF,     // %011 11

          // ECM=1 BMM=0 MCM=0
          // Extended Background Color Mode (only care about color bit 1)
          VIC_COL_ECM,      // %100 00
          VIC_COL_ECM,      // %100 01
          VIC_COL_CBUF,     // %100 10
          VIC_COL_CBUF,     // %100 11

          // ECM=1 BMM=0 MCM=1
          // Extended Background Color Multicolor Character Mode (invalid)
          VIC_COL_NONE,     // %101 00
          VIC_COL_NONE,     // %101 01
          VIC_COL_NONE,     // %101 10
          VIC_COL_NONE,     // %101 11

          // ECM=1 BMM=1 MCM=0
          // Extended Background Color Standard Bitmap Mode (invalid)
          VIC_COL_NONE,     // %110 00
          VIC_COL_NONE,     // %110 01
          VIC_COL_NONE,     // %110 10
          VIC_COL_NONE,     // %110 11

          // ECM=1 BMM=1 MCM=1
          // Extended Background Color Multicolor Bitmap Mode (invalid)
          VIC_COL_NONE,      // %111 00
          VIC_COL_NONE,      // %111 00
          VIC_COL_NONE,      // %111 00
          VIC_COL_NONE       // %111 00
    };

// hold color for each video mode
uint8_t vic_videoModeColors[10];

// ABGR8888 values for each of the 16 colors
uint32_t vic_colors[16];

// 8 sprites
sprite_t vic_sprites[8];

// linked list of sprites currently being displayed
sprite_t vic_spriteLinkedListHead;

// color data for 40 columns
uint8_t vic_colorData[40];

// character data for 40 columns
uint8_t vic_videoMatrixData[40];

// pixel buffers for the whole screen
uint32_t vic_pixels[VIC_PIXELS_LENGTH];
uint32_t vic_pixelBuffer[VIC_PIXELS_LENGTH];


uint32_t vic_borderColor;

// the last value read by the vic
uint8_t vic_phi1Data = 0;

// the pixel color being rendered
uint32_t vic_pixelColor = 0;

/** multicolor flip-flop state */
bool_t vic_mcFlip = false;
/** xscroll-delayed phi1 data */
uint32_t vic_phi1DataPipe = 0;

/*

From http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt:

3.7.2. VC and RC
normally VC counts all 1000 addresses of the video
matrix within the display frame and that RC counts the 8 pixel lines of
each text line. The behavior of VC and RC is largely determined by Bad Line
Conditions which you can control with the processor via YSCROLL, giving you
control of the VC and RC within certain limits.
*/

// "VC" (video counter) is a 10 bit counter that can be loaded with the value from VCBASE.
// normally counts the 1000 addresses of the video matrix (40x25)
int32_t vic_vc = 0;

// "VCBASE" (video counter base) is a 10 bit data register with reset input that can be loaded with the value from VC.
int32_t vic_vcBase = 0;

// "RC" (row counter) is a 3 bit counter (0-7) with reset input.
// counts the vertical position in each group of 8 pixels
int32_t vic_rc = 0;


bool_t vic_isDisplayActive = false;

/** are bad lines enabled for this frame? */
bool_t vic_areBadLinesEnabled = false;

// current raster line
int32_t vic_rasterY = 0;

/** Is rasterYIRQ condition true? */
bool_t vic_rasterYIRQCondition = false;
bool_t vic_showBorderVertical = false;
bool_t vic_showBorderMain = false;
bool_t vic_isBadLine = false;

int32_t vic_videoMatrixBase = 0;
int32_t vic_charMemBase = 0;
int32_t vic_bitmapMemBase = 0;

/** vertical scrolling value */
int32_t vic_yscroll = 0;
/** horizontal scrolling value */
int8_t vic_xscroll = 0;

int32_t vic_latchedXscroll = 0;

uint8_t vic_irqFlags = 0;
uint8_t vic_irqMask = 0;
/** Index of next pixel to paint */
uint32_t vic_nextPixel = 0;

// line cycle is incremented on each cycle
// loops from 1 to CYCLES_PER_LINE
// in PAL (6569) line cycle goes up to 63
// in NTSC (6567) line cycle goes up to 65
int32_t vic_cycle = 0;


bool_t vic_graphicsRendering = false;
/** Light pen coordinates */  
uint8_t vic_lpx = 0;
uint8_t vic_lpy = 0;
bool_t vic_lpTriggered = false;

bool_t vic_startOfFrame = false;
int32_t vic_MAX_RASTERS = 0;
bool_t vic_lpAsserted = false;

// latched colour is the color in the colour buffer
// for the current cycle
uint8_t vic_latchedColor = 0;
uint8_t vic_latchedVmd = 0;
uint32_t vic_oldGraphicsData = 0;

/** Number of cycles per line. */
int32_t vic_CYCLES_PER_LINE;

/** Number of raster lines */
int32_t vic_MAX_RASTERS;

int32_t vic_videoModeColorDecoderOffset = 0;


// events..
event_t vic_makeDisplayActive;
void vic_makeDisplayActive_function(void *context) {
  vic_isDisplayActive = true;    
}


event_t vic_badLineStateChange;
void vic_badLineStateChange_function(void *context) {
  vic_setBA(!vic_isBadLine);  
}



event_t vic_rasterYIRQEdgeDetector;
void vic_rasterYIRQEdgeDetector_function(void *context) {
  bool_t oldRasterYIRQCondition = vic_rasterYIRQCondition;

  // check if current raster line = raster interrupt line
  // 0xd012 Read: Raster Interrupt Line (bits #0-#7).
  // 0xd011 Read: Bit #7: Raster Interrupt Line (bit #8).
  vic_rasterYIRQCondition = vic_rasterY == ((vic_registers[0x12]) + ((vic_registers[0x11] & 0x80) << 1));

  // did irq condition switch from false to true?
  if (!oldRasterYIRQCondition && vic_rasterYIRQCondition) {
    vic_activateIRQFlag(VIC_IRQ_RASTER);
  }
}


// table used to quadruple bits of 4 bit numbers
// high 16 bits stores the inverse
// eg %1011 -> %00001111000000001111000011111111 
uint32_t quadrupleBits[16];

void init_quadrupleBits() {
  uint32_t i,b;
  uint32_t out;

  // quadruple each bit
  for (i = 0; i < 16; i++) {
    out = 0;
    for (b = 0; b < 4; b++) {
      if ((i & (1 << b)) != 0) {
          out |= 0xf << (b << 2);
      }
    }

    // high bits are the inverse of the lower bits
    quadrupleBits[i] = out | (0xffff ^ out) << 16;
  }  
}


// set the value for a color (ABGR8888) Alpha is highest byte, Red is Lowest 
void m64_setColor(int32_t index, uint32_t color) {
  if(index >= 0 && index < 16) {
    vic_colors[index] = color;
  }
}

void vic_initPALColors() {
  // default colodore palette
  // https://www.colodore.com/
  vic_colors[0] = 0xFF000000;
  vic_colors[1] = 0xFFffffff;
  vic_colors[2] = 0xFF383381;
  vic_colors[3] = 0xFFc8ce75;
  vic_colors[4] = 0xFF973c8e;
  vic_colors[5] = 0xFF4dac56;
  vic_colors[6] = 0xFF9b2c2e;
  vic_colors[7] = 0xFF71f1ed;
  vic_colors[8] = 0xFF29508e;
  vic_colors[9] = 0xFF003855;
  vic_colors[10] = 0xFF716cc4;
  vic_colors[11] = 0xFF4a4a4a;
  vic_colors[12] = 0xFF7b7b7b;
  vic_colors[13] = 0xFF9fffa9;
  vic_colors[14] = 0xFFeb6d70;
  vic_colors[15] = 0xFFb2b2b2;
}

void vic_initNTSCColors() {
  // colors for machines with the SONY CXA2025AS US decoder matrix
  // used in some ntsc models
  // colors from vice 'pepto-ntsc-sony.vpl', ignoring dither values

  vic_colors[0] = 0xFF000000;
  vic_colors[1] = 0xFFffffff;
  vic_colors[2] = 0xFF2b357c;
  vic_colors[3] = 0xFFb1a65a;
  vic_colors[4] = 0xFF854169;
  vic_colors[5] = 0xFF43865d;
  vic_colors[6] = 0xFF782e21;
  vic_colors[7] = 0xFF6fbecf;
  vic_colors[8] = 0xFF264a89;
  vic_colors[9] = 0xFF00335b;
  vic_colors[10] = 0xFF5964af;
  vic_colors[11] = 0xFF434343;
  vic_colors[12] = 0xFF6b6b6b;
  vic_colors[13] = 0xFF84cba0;
  vic_colors[14] = 0xFFb36556;
  vic_colors[15] = 0xFF959595;
}

void vic_init(int32_t model) {
  uint32_t i;

  init_quadrupleBits();
  for (i = 0; i < VIC_SPRITECOUNT; i++) {
    sprite_init(&(vic_sprites[i]), &vic_spriteLinkedListHead, i);
  }

  vic_makeDisplayActive.event = &vic_makeDisplayActive_function;
  vic_badLineStateChange.event = &vic_badLineStateChange_function;
  vic_rasterYIRQEdgeDetector.event = &vic_rasterYIRQEdgeDetector_function;

  vic_model = model;
  if(model == VIC_MODEL6567R8) {
    // ntsc
    vic_initNTSCColors();
    vic_CYCLES_PER_LINE = 65;
    vic_MAX_RASTERS = 263;
    m6567_init();
  } else {
    // pal
    vic_initPALColors();
    vic_CYCLES_PER_LINE = 63;
    vic_MAX_RASTERS = 312;
    m6569_init();
  }


}

// return sprite x coordinate
int32_t vic_readSpriteXCoordinate(int32_t spriteIndex) {
  // the lower 8 bits of sprite x coordinates are stored in
  // d000, d002, d004, d006, d008, etc
  // the ninth bit is stored in corresponding bit in d010
  return (vic_registers[(spriteIndex << 1)] & 0xff) 
    + ((vic_registers[0x10] & 1 << spriteIndex) != 0 ? 0x100 : 0);
}



/*
  The height and width of the display window can each be set to two different
  values with the bits RSEL and CSEL in the registers $d011 and $d016:

  RSEL|  Display window height   | First line  | Last line
  ----+--------------------------+-------------+----------
    0 | 24 text lines/192 pixels |   55 ($37)  | 246 ($f6)
    1 | 25 text lines/200 pixels |   51 ($33)  | 250 ($fa)

  CSEL|   Display window width   | First X coo. | Last X coo.
  ----+--------------------------+--------------+------------
    0 | 38 characters/304 pixels |   31 ($1f)   |  334 ($14e)
    1 | 40 characters/320 pixels |   24 ($18)   |  343 ($157)

  If RSEL=0 the upper and lower border are each extended by 4 pixels into the
  display window, if CSEL=0 the left border is extended by 7 pixels and the
  right one by 9 pixels. The position of the display window and its
  resolution do not change, RSEL/CSEL only switch the starting and ending
  position of the border display. The size of the video matrix also stays
  constantly at 40×25 characters.
*/

// return the RSEL (Row Select) Flag
// true  = 25 rows
// false = 24 rows
bool_t vic_readRSEL() {
  // d011 Bit #3: Screen height; 
  // 0 = 24 rows; 1 = 25 rows.
  return (vic_registers[0x11] & 8) != 0;
}


// return the CSEL (Column Select) Flag
// true  = 40 columns
// false = 38 columns
bool_t vic_readCSEL() {
  // d016: Bit #3: Screen width; 
  // 0 = 38 columns; 1 = 40 columns.
  return (vic_registers[0x16] & 8) != 0;
}


// return the DEN (Display Enable) flag
// true  = Screen On
// false = Screen Off
bool_t vic_readDEN() {
  // d011 Bit #4: 
  // 0 = Screen off, complete screen is covered by border; 
  // 1 = Screen on, normal screen contents are visible.
  return (vic_registers[0x11] & 0x10) != 0;
}

/*
 A Bad Line Condition is given at any arbitrary clock cycle, if at the
 negative edge of ø0 at the beginning of the cycle RASTER >= $30 and RASTER
 <= $f7 and the lower three bits of RASTER are equal to YSCROLL and if the
 DEN bit was set during an arbitrary cycle of raster line $30.
*/ 
bool_t vic_evaluateIsBadLine() {
  return vic_areBadLinesEnabled 
          && vic_rasterY >= VIC_FIRST_DMA_LINE 
          && vic_rasterY <= VIC_LAST_DMA_LINE 
          && (vic_rasterY & 0x7) == vic_yscroll;
}




/**
 * Signal CPU interrupt if requested by VIC_
 */
void vic_handleIrqState() {
  if ((vic_irqFlags & vic_irqMask & 0x0f) != 0) {
    if ((vic_irqFlags & 0x80) == 0) {
      vic_interrupt(true);
      vic_irqFlags |= 0x80;
    }
  } else if ((vic_irqFlags & 0x80) != 0) {
    vic_interrupt(false);
    vic_irqFlags &= 0x7f;
  }
}

/**
 * Get the video memory base address, which is determined by the inverted
 * bits 0-1 of port A on CIA 2, plus the video matrix base address plus the
 * character data base address plus the bitmap memory base address.
 */
void vic_determineVideoMemoryBaseAddresses() {
  // d018 Bits #4-#7: Pointer to screen memory (bits #10-#13), relative to VIC bank
  vic_videoMatrixBase = (vic_registers[0x18] & 0xf0) << 6;

  // d018 Bits #1-#3: In text mode, pointer to character memory (bits #11-#13), relative to VIC bank
  vic_charMemBase = (vic_registers[0x18] & 0x0e) << 10;

  // d018 In bitmap mode, pointer to bitmap memory (bit #13), relative to VIC bank
  vic_bitmapMemBase = (vic_registers[0x18] & 0x08) << 10;
}


/**
 * Set an IRQ flag and trigger an IRQ if the corresponding IRQ mask is set.
 * The IRQ only gets activated, i.e. flag 0x80 gets set, if it was not
 * active before.
 */
void vic_activateIRQFlag(uint8_t flag) {
  vic_irqFlags |= flag;
  vic_handleIrqState();
}


void vic_doVideoMatrixAccess() {
  // video matrix access starts on phi2 of cycle 15 (both pal and ntsc), so subtract 15 to get video matrix index
  int32_t displayCycle = vic_cycle - 15;
  vic_videoMatrixData[displayCycle] = vic_vicReadMemoryPHI2(vic_videoMatrixBase | vic_vc);
  vic_colorData[displayCycle] = vic_vicReadColorMemoryPHI2(vic_vc);
}

void vic_drawSpritesAndGraphics() {
  int32_t pixel = 0;
  int32_t end;
  int32_t color;
  
  uint32_t mask;
  uint32_t shift, out, priorityBits;
  uint32_t inputBits, videoModeColorsSIMD;
  uint32_t bits;


  uint32_t otherSprite;
  
  uint32_t i,j;
  uint32_t val;

  // column 0 is rendered cycle 16-2/17-1 (both pal and ntsc)
  int32_t renderCycle = vic_cycle - 17;
  if (renderCycle < 0) {
    renderCycle += vic_CYCLES_PER_LINE;
  }

  // 4-bits per pixel, set to 0xf when occupied
  uint32_t priorityData = 0;

  // 4-bits per pixel, each 4 bits has the color (0-f)
  uint32_t graphicsDataBuffer = 0;
  uint32_t graphicsDataBufferSave = 0;

  for (pixel = 0; pixel < 32;) {

    /* At midpoint -> set video mode bits. */    
    if (pixel == 16) {
      // set the video mode decoder offset
      // check bit 6 and 5 of d011 ( bit 6 = extended background mode and bit 5 bitmap mode)
      // and bit 4 of d016 ( bit 4 = multicolor mode )
      vic_videoModeColorDecoderOffset |= ( ( (vic_registers[0x11] & 0x60) | (vic_registers[0x16] & 0x10) ) >> 2);
    }

    // if pixel equals latched x scroll, load new data    
    if (pixel == vic_latchedXscroll) {
      // color from color buffer 
      vic_videoModeColors[VIC_COL_CBUF] = vic_latchedColor;
      // multicolor from color buffer
      vic_videoModeColors[VIC_COL_CBUF_MC] = (vic_latchedColor & 0x07) ;

      // video matrix low/high nibbles
      vic_videoModeColors[VIC_COL_VBUF_L] = (vic_latchedVmd & 0x0f) ;
      vic_videoModeColors[VIC_COL_VBUF_H] = (vic_latchedVmd >> 4 & 0x0f);

      vic_videoModeColors[VIC_COL_ECM] = vic_videoModeColors[VIC_COL_D021 + ((vic_latchedVmd >> 6) & 0x03)];
      vic_mcFlip = true;

      // if in a cycle for columns 0-39
      if (renderCycle < 40 && !vic_showBorderVertical) {
        vic_latchedVmd = vic_isDisplayActive ? vic_videoMatrixData[renderCycle] : 0;

        // colorData contains the color of the current character (array size 40)
        vic_latchedColor = vic_isDisplayActive ? vic_colorData[renderCycle] : 0;

        // phi1data is the last value read by the vic (g-access)
        // in character mode these are the bits for the current character 
        vic_phi1DataPipe ^= (vic_phi1DataPipe ^ vic_phi1Data << 16) & 0xff0000;
      }
    }

    // Calculate size of renderable chunk: either until next 16 bits, or to next xscroll.
    end = pixel + 16 & 0xf0;
    if (pixel < vic_latchedXscroll) {
      if(vic_latchedXscroll < end) {
        end = vic_latchedXscroll;
      }
    }

    // if bit 2 of vic_videoModeColorDecoderOffset is set and the cbuf video mode color is 8, then it's multicolor mode
    if ((vic_videoModeColorDecoderOffset & 4) != 0  && !(vic_videoModeColorDecoderOffset == 4 && vic_videoModeColors[VIC_COL_CBUF] < 8)) {

      // multicolor mode
      while ((pixel < end)) {
        
        // for multicolor read pixel color every 2 pixels
        if (vic_mcFlip) {
          vic_pixelColor = vic_phi1DataPipe >> 30;
        }
        vic_mcFlip = !vic_mcFlip;

        // shift data by a pixel
        vic_phi1DataPipe <<= 1;
        graphicsDataBuffer <<= 4;
        priorityData <<= 4;

        // convert 2bit vic_pixelColor into video mode using the video mode color decoder
        color = vic_videoModeColorDecoder[vic_videoModeColorDecoderOffset | vic_pixelColor];

        // get the 0-15 color for the video mode
        // and stick it into graphics data buffer
        // graphics data buffer has 4bits per color
        graphicsDataBuffer |= vic_videoModeColors[color];

        // priority data for pixel is 0xf if pixel is set
        priorityData |= vic_pixelColor > 1 ? 0xf : 0;

        pixel += 4;
      };
    } else {

      // not multicolour
      bits = end - pixel;
      //console.log(bits);
      // if bits == 16, mask = 0xffff
      // if bits == 12, mask = 0xfff
      // if bits == 8, mask = 0xff
      // if bits == 4, mask = 0xf

      mask = 0xffffffff;
      shift = 0xffffffff - bits + 1;
      mask = mask >> shift;


      /* Extract bits of input */
      inputBits = vic_phi1DataPipe >> (-bits >> 2);
      vic_phi1DataPipe <<= bits >> 2;
      vic_mcFlip ^= (bits & 4) != 0;
      videoModeColorsSIMD =   (vic_videoModeColors[vic_videoModeColorDecoder[vic_videoModeColorDecoderOffset]] << 16)
                            | (vic_videoModeColors[vic_videoModeColorDecoder[vic_videoModeColorDecoderOffset | 3]] & 0xff);
      videoModeColorsSIMD |= videoModeColorsSIMD << 4;
      videoModeColorsSIMD |= videoModeColorsSIMD << 8;

      /* Get decoded value of 0xABCDabcd representing 4-bit input. */
      priorityBits = quadrupleBits[inputBits];

      /* Generate BG and FG simultaneously. */
      out = videoModeColorsSIMD & priorityBits;

      /* Merge */
      out |= out >> 16;

      /* Place merged data where it is wanted. */
      graphicsDataBuffer <<= bits;
      graphicsDataBuffer |= out & mask;

      /* Separate data channel for sprite priority handling */
      priorityData <<= bits;
      priorityData |= priorityBits & mask;
      pixel = end;
    }
  }

  /*
   * This should happen on the 6th, 7th or such pixel. It's apparently
   * related to the fall time in NMOS, and can be observed to change with
   * system temperature.
   */

  // set video mode color decoder according to vic registers
  // check bit 6 and 5 of d011 ( bit 6 = extended background mode and bit 5 bitmap mode)
  // and bit 4 of d016 ( bit 4 = multicolor mode )
  vic_videoModeColorDecoderOffset &= (( (vic_registers[0x11] & 0x60) | (vic_registers[0x16] & 0x10) ) >> 2);

  // now render sprites

  uint32_t opaqueSpritePixels = 0;
  uint32_t spriteForegroundMask;
  sprite_t *prev = &vic_spriteLinkedListHead;
  sprite_t *current = prev->nextVisibleSprite;

  graphicsDataBufferSave = graphicsDataBuffer;

  while ((current != NULL)) {
    
    spriteForegroundMask = sprite_calculateNext8Pixels(current);

    // check sprite-background collision
    if ((spriteForegroundMask & priorityData) != 0) {
      if (vic_registers[31] == 0) {
        vic_activateIRQFlag(VIC_IRQ_SPRITE_BACKGROUND_COLLISION);
      }
      vic_registers[31] |= 1 << current->index;
    }

    uint32_t priorityMask = sprite_getNextPriorityMask(current);

    /*
     * Handle sprite-sprite collision. It's easy to detect that a
     * collision occurred, but hard to find which two sprites collided.
     * For this purpose, we store the sprite index which contributed a
     * pixel into the low bits of a pixel slot, and keep the 4th bit as
     * a mask that we can use to reliably identify the collision.
     */    
    if ((opaqueSpritePixels & spriteForegroundMask) != 0) {
      if(priorityMask == 0) {
        // if this sprite is below the foreground, put foreground in graphics data buffer
        graphicsDataBuffer = (graphicsDataBuffer & ~spriteForegroundMask) | (graphicsDataBufferSave & spriteForegroundMask);
      }

      if (vic_registers[0x1e] == 0) {
        vic_activateIRQFlag(VIC_IRQ_SPRITE_SPRITE_COLLISION);
      }
      vic_registers[0x1e] |= 1 << current->index;
      for (pixel = 0; pixel < 32; pixel += 4) {
        /* non-transparent from us? */
        if ((spriteForegroundMask >> pixel & 0xf) == 0) {
          continue;
        }

        /* non-transparent from other? */        
        otherSprite = opaqueSpritePixels >> pixel & 0xf;
        if (otherSprite == 0) {
          /* no pixel set at that slot? Set ourselves */          
          opaqueSpritePixels |= (current->index | 8) << pixel;
        } else {
          /*
           * Collision; register the other sprite as colliding,
           * but keep the old value, as it doesn't matter whether
           * it contains the other or ourselves: both collision
           * bits are set on 0x1e once we finish.
           */          
          vic_registers[0x1e] |= 1 << (otherSprite & 7);
        }
      }
    } else {
      opaqueSpritePixels |= current->indexBits & spriteForegroundMask;
    }

    spriteForegroundMask &= ~priorityData | priorityMask;
    graphicsDataBuffer ^= (current->colorBuffer ^ graphicsDataBuffer) & spriteForegroundMask;
    if (current->consuming) {
      prev = current;
    } else {
      prev->nextVisibleSprite = current->nextVisibleSprite;
    }
    current = current->nextVisibleSprite;
  }


  // Border Unit
  if ((renderCycle == 1 || renderCycle == 39) && !vic_readCSEL()) {
    if (vic_showBorderMain) {
      graphicsDataBuffer ^= (graphicsDataBuffer ^ vic_borderColor) & 0xfffffff0;
    }

    vic_showBorderMain = vic_showBorderVertical || renderCycle == 39;

    if (vic_showBorderMain) {
      //graphicsDataBuffer ^= (graphicsDataBuffer ^ vic_borderColor) & 15;
      graphicsDataBuffer ^= (graphicsDataBuffer ^ vic_borderColor) & 0x0000000f;
    }
  } else {
    if (vic_showBorderMain) {
      graphicsDataBuffer = vic_borderColor;
    }
    if ((renderCycle == 0 || renderCycle == 40) && vic_readCSEL()) {
      vic_showBorderMain = vic_showBorderVertical || renderCycle == 40;
    }
  }


 


  /* Pixels arrive in 0x12345678 order. */  
  // each pixel is 4 bits.. 0-15

  // int graphicsDataBuffer = 0;
  // int oldGraphicsData;
  // oldGraphicsData =  Previous sequencer data 
  // The unsigned right shift operator ">>>" shifts a zero into the leftmost position
  // a >>> b    Shifts a in binary representation b (< 32) bits to the right, discarding bits shifted off, and shifting in 0s from the left.

  for (j = 0; j < 2; j++) {
    //vic_oldGraphicsData |= graphicsDataBuffer >>> 16;
    vic_oldGraphicsData |= graphicsDataBuffer >> 16;
    // first loop, set oldGraphicsData to 0x1234
    for (i = 0; i < 4; i++) {
      vic_oldGraphicsData <<= 4;

      val = (vic_oldGraphicsData >> 16) & 0xf;

      vic_pixels[vic_nextPixel++] = vic_colors[val];//color;


    }
    graphicsDataBuffer <<= 16;
  }
}



/**
* This version just detects sprite-sprite collisions. It is appropriate to
* use outside renderable screen, where graphics sequencer is known to have
* quiesced.
*/
void vic_spriteCollisionsOnly() {
  uint32_t opaqueSpritePixels = 0;
  sprite_t *prev = &vic_spriteLinkedListHead;
  sprite_t *current = prev->nextVisibleSprite;
  uint32_t spriteForegroundMask, pixel, otherSprite;

  while ((current != NULL)) {
    spriteForegroundMask = sprite_calculateNext8Pixels(current);

    if ((opaqueSpritePixels & spriteForegroundMask) != 0) {
      if (vic_registers[0x1e] == 0) {
        vic_activateIRQFlag(VIC_IRQ_SPRITE_SPRITE_COLLISION);
      }
      vic_registers[0x1e] |= 1 << current->index;
      for (pixel = 0; pixel < 32; pixel += 4) {
        if ((spriteForegroundMask >> pixel & 1) == 0) {
          continue;
        }
        otherSprite = opaqueSpritePixels >> pixel & 15;
        if (otherSprite == 0) {
          opaqueSpritePixels |= (current->index | 8) << pixel;
        } else {
          vic_registers[0x1e] |= 1 << (otherSprite & 7);
        }
      }
    } else {
      opaqueSpritePixels |= current->indexBits & spriteForegroundMask;
    }

    if (current->consuming) {
      prev = current;
    } else {
      prev->nextVisibleSprite = current->nextVisibleSprite;
    }
    current = current->nextVisibleSprite;
  }
}
/**
* In certain cases, CPU sees the stale bus data from VIC. VIC reads on
* every cycle, and this describes what it reads.
*/
uint8_t vic_getLastReadByte() {
  return vic_phi1Data;
}



// each sprite data access is over 2 cycles if enabled
// cycle 1, phi 1: read sprite pointer
// cycle 1, phi 2: read sprite byte into upper 8 bits of sprite shift register, increment mc
// cycle 2, phi 1: read sprite byte into middle 8 bits of sprite shift register, increment mc
// cycle 2, phi 2: read sprite byte into lower 8 bits of sprite shift register, increment mc
void vic_fetchSpritePointer(int32_t n) {
  sprite_t *sprite = &(vic_sprites[n]);
  int32_t x;

  sprite_setPointerByte(sprite, vic_phi1Data);
  x = sprite_getX(sprite);
  if (x >= 352 + 16 * n && x <= 359 + 16 * n) {
    sprite_event(sprite);
  }
  sprite_repeatPixels(sprite);
  sprite_setLineByte(sprite, 2, vic_vicReadMemoryPHI2(sprite_getCurrentByteAddress(sprite )));
}


// called one cycle after fetchSpritePointer
// set the remaining sprite bytes for the line
// call handleSpriteVisibilityEvent to determine the cycle the sprite should be displayed
// and schedule an event to start displaying the sprite on that cycle
void vic_fetchSpriteData(int32_t n) {
  sprite_t *sprite = &(vic_sprites[n]);
  int32_t x;

  sprite_setLineByte(sprite, 1, vic_phi1Data);
  sprite_setLineByte(sprite, 0, vic_vicReadMemoryPHI2(sprite_getCurrentByteAddress(sprite)));
  x = sprite_getX(sprite);
  if (x == 367 + 16 * n) {
    sprite_event(sprite);
  } else {
    vic_handleSpriteVisibilityEvent(sprite);
  }
}

// read a vic register
uint8_t vic_read(uint16_t reg) {
  uint8_t value;

  // registers go up to 0x3f
  reg &= 0x3f;
  
  switch (reg) {
    case VIC_CONTROL1: // 0xd011:
      {
        value = (uint8_t) ((vic_registers[reg] & 0x7f) | (vic_rasterY & 0x100) >> 1);
        break;
      }

    case VIC_RASTERCOUNTER: //0xd012:
      value = (uint8_t) (vic_rasterY & 0xff);
      break;
    case VIC_LIGHTPENX: //0xd013:
      value = vic_lpx;
      break;
    case VIC_LIGHTPENY: //0xd014:
      value = vic_lpy;
      break;
    case VIC_CONTROL2: //0xd016:
      // vic control register
      // unused bits 6, 7 always 1
      value = (uint8_t)(vic_registers[reg] | 0xc0);
      break;
    case VIC_MEMORYSETUP: //0xd018:
      // unused bit is always 1
      value = (uint8_t)(vic_registers[reg] | 0x01);
      break;
    case VIC_INTERRUPT: //0xd019:
      // Interrupt Pending Register
      value = (uint8_t)(vic_irqFlags | 0x70);
      break;
    case VIC_INTERRUPTENABLED: //0xd01a:
      // Interrupt Mask Register
      value = (uint8_t)(vic_irqMask | 0xf0);
      break;
    case VIC_SPRITESPRITECOLLISION: // 0xd01e:
    case VIC_SPRITEDATACOLLISION: // 0xd01f:
      // register is cleared when read
      value = vic_registers[reg];
      vic_registers[reg] = 0;
      break;
    default:
      if (reg < 0x20) {
        value = vic_registers[reg];
      } else if (reg < 0x2f) {
        // border/bg/sprite colors are only lower 4 bits (0-15) with upper 4bits set to 1
        value = (uint8_t)(vic_registers[reg] | 0xf0);
      } else {
        // The unused addresses $d02f-$d03f give $ff on reading, a write access is ignored
        value = (uint8_t)0xff;
      }
  }

  value = value & 0xff;

  // if the cpu reads from the vic while sprite data is 
  // being read, sprite data is damaged
  vic_setSpriteDataFromCPU(value);
  return value;
};


/**
* Handle lightpen state change
*/
void vic_lightpenEdgeDetector() {
  if (vic_rasterY != vic_MAX_RASTERS - 1) {
    if (!vic_lpAsserted) {
      return;
    }

    if (vic_lpTriggered) {
      return;
    }

    vic_lpTriggered = true;
    vic_lpx = vic_getCurrentSpriteCycle();
    vic_lpx++;

    if (vic_lpx == vic_CYCLES_PER_LINE) {
      vic_lpx = 0;
    }
    vic_lpx <<= 2;
    vic_lpx += clock_getPhase(&m64_clock) == PHASE_PHI1 ? 1 : 2;
    vic_lpy = (vic_rasterY & 0xff);
    if(vic_cycle == vic_CYCLES_PER_LINE) {
      vic_lpy++;
    }
    vic_activateIRQFlag(VIC_M656X_INTERRUPT_LP);
  }
};


// write to a vic register
void vic_write(uint16_t reg, uint8_t data) {
  uint32_t m, n, i;
  int32_t renderCycle;
  int32_t narrowing;
  bool_t oldBadLine;
  bool_t expandY;

  reg &= 0x3f;
  vic_registers[reg] = data;

  // if the cpu writes to the vic while sprite data is being read by the vic
  // sprite is damaged
  vic_setSpriteDataFromCPU(data);

  switch (reg) {
    // sprite x coordinate registers
    case VIC_SPRITE0X: // 0xd000
    case VIC_SPRITE1X: // 0xd002
    case VIC_SPRITE2X: // 0xd004
    case VIC_SPRITE3X: // 0xd006
    case VIC_SPRITE4X: // 0xd008
    case VIC_SPRITE5X: // 0xd00a
    case VIC_SPRITE6X: // 0xd00c
    case VIC_SPRITE7X: // 0xd00e
      n = reg >> 1;
      sprite_setX(&(vic_sprites[n]), vic_readSpriteXCoordinate(n));
      vic_handleSpriteVisibilityEvent(&(vic_sprites[n]));
      break;
    // sprite y coordinate registers
    case VIC_SPRITE0Y: // 0xd001
    case VIC_SPRITE1Y: // 0xd003
    case VIC_SPRITE2Y: // 0xd005
    case VIC_SPRITE3Y: // 0xd007
    case VIC_SPRITE4Y: // 0xd009
    case VIC_SPRITE5Y: // 0xd00b
    case VIC_SPRITE6Y: // 0xd00d
    case VIC_SPRITE7Y: // 0xd00f
        sprite_setY(&(vic_sprites[reg >> 1]),  data & 0xff);
        break;
    case VIC_SPRITEXMSB:  // 0xd010
      // sprite x coordinate msb regsiter
      // bit 9 of sprite x coordinate
      for (i = 0; i < 8; i++) {
          sprite_setX(&(vic_sprites[i]), vic_readSpriteXCoordinate(i));
          vic_handleSpriteVisibilityEvent(&(vic_sprites[i]));
      }
      break;
    case VIC_CONTROL1: // 0xd011
        vic_yscroll = data & 0x7;

        if (vic_rasterY == VIC_FIRST_DMA_LINE) {
            vic_areBadLinesEnabled = vic_areBadLinesEnabled || vic_readDEN();
        }

        // can only make changes if not on last cycle
        if(vic_cycle != vic_CYCLES_PER_LINE) {
          narrowing = vic_readRSEL() ? 0 : 4;
          if (vic_rasterY == VIC_FIRST_DMA_LINE + 3 + narrowing && vic_readDEN()) {
            vic_showBorderVertical = false;
          }
          if (vic_rasterY == VIC_LAST_DMA_LINE + 4 - narrowing) {
            vic_showBorderVertical = true;
          }
        }

        // if changes from not badline to badline, schedule make display active
        oldBadLine = vic_isBadLine;
        vic_isBadLine = vic_evaluateIsBadLine();
        if (!oldBadLine && vic_isBadLine) {
          clock_scheduleEvent(&m64_clock, &vic_makeDisplayActive, 1, PHASE_PHI2);
        }

        // if badline state has changed, schedule badline state change
        if ((vic_isBadLine != oldBadLine) && vic_cycle > 11 && vic_cycle < 54) {  
          clock_scheduleEvent(&m64_clock, &vic_badLineStateChange, 0, PHASE_PHI1);
        }

        // fall through as bit 8 of current raster line has just been set
    case VIC_RASTERCOUNTER: // 0xd012  
        // 0xd012 = bottom 8 bits of raster line where to generate interrupts
        // schedule an event to check raster Y irq condition changes at the next PHI1 
        clock_scheduleEvent(&m64_clock, &vic_rasterYIRQEdgeDetector, 0, PHASE_PHI1);
        break;
    case VIC_SPRITEENABLED: //0xd015
        for (i = 0; i < 8; i++) {
            sprite_setEnabled(&(vic_sprites[i]), (data & 1 << i) != 0);
        }
        break;
    case VIC_CONTROL2: //0xd016
        vic_xscroll = (vic_registers[0x16] & 7);

        renderCycle = vic_cycle - 17;
        if (renderCycle <= 0) {
          renderCycle += vic_CYCLES_PER_LINE;
        }
        if (renderCycle != 39) {
          vic_latchedXscroll = vic_xscroll << 2;
        }
        break;
    case VIC_SPRITEEXPY: // 0xd017
        for (i = 0; i < 8; i++) {
          expandY = (data & 1 << i) != 0;
          sprite_setExpandY(&(vic_sprites[i]), expandY, vic_cycle == 15);
        }
        break;
    case VIC_MEMORYSETUP: // 0xd018
        vic_determineVideoMemoryBaseAddresses();
        break;
    case VIC_INTERRUPT: //0xd019
        vic_irqFlags &= (~data & 15) | 128;
        vic_handleIrqState();
        break;
    case VIC_INTERRUPTENABLED: //0xd01a
        vic_irqMask = (data & 0xf);
        vic_handleIrqState();
        break;
    case VIC_SPRITEDATAPRIORITY: //0xd01b
      // the sprite priority byte has changed        
      for (i = 0, m = 1; i < 8; i++, m <<= 1) {
          sprite_setPriorityOverForegroundGraphics(&(vic_sprites[i]), (data & m) == 0);
      }
      break;
    case VIC_SPRITEMULTICOLOR: //0xd01c
      for (i = 0, m = 1; i < 8; ++i, m <<= 1) {
          sprite_setMulticolor(&(vic_sprites[i]), (data & m) != 0);
      }
      break;
    case VIC_SPRITEEXPX: // 0xd01d
      for (i = 0, m = 1; i < 8; i++, m <<= 1) {
          sprite_setExpandX(&(vic_sprites[i]), (data & m) != 0);
      }
      break;
    case VIC_BORDERCOLOR: // 0xd020
      // the border color was changed
      vic_borderColor = (data & 0xf) * 0x11111111;
      break;
    case VIC_BGCOLOR0: //0xd021
    case VIC_BGCOLOR1: //0xd022
    case VIC_BGCOLOR2: //0xd023
    case VIC_BGCOLOR3: //0xd024
      //data = 5;
      n = reg - 0x21;
      vic_videoModeColors[n] = (data & 15);
      break;
    case VIC_SPRITEMULTICOLOR0: //37:
        for (i = 0; i < 8; i++) {
          sprite_setColor(&(vic_sprites[i]), 1, (data & 0xf) );
        }
        vic_registers[reg] |= 0xf0;
        break;
    case VIC_SPRITEMULTICOLOR1: //0xd026
        for (i = 0; i < 8; i++) {
          sprite_setColor(&(vic_sprites[i]), 3, (data & 0xf) );
        }
        vic_registers[reg] |= 0xf0;
        break;
    case VIC_SPRITECOLOR0: //0xd027
    case VIC_SPRITECOLOR1: //0xd028
    case VIC_SPRITECOLOR2: //0xd029
    case VIC_SPRITECOLOR3: //0xd02a
    case VIC_SPRITECOLOR4: //0xd02b
    case VIC_SPRITECOLOR5: //0xd02c
    case VIC_SPRITECOLOR6: //0xd02d
    case VIC_SPRITECOLOR7: //0xd02e
      sprite_setColor(&(vic_sprites[reg - 0x27]), 2, (data & 0xf) );
      vic_registers[reg] |= 0xf0;
      break;
  }
}

// If CPU reads/writes to VIC at just the cycle VIC is supposed to do a PHI2
// fetch for sprite data, the data fetched is set from the interaction with
// CPU.
void vic_setSpriteDataFromCPU(uint8_t data) {
  sprite_t *damagedSprite;
  
  if( ( vic_cycle >= 1  && vic_cycle < 11) || vic_cycle >= ( vic_CYCLES_PER_LINE - 5)) {
    uint32_t spriteIndex = ( (vic_cycle + 5) % vic_CYCLES_PER_LINE) >> 1;
    damagedSprite = &(vic_sprites[spriteIndex]);
    sprite_setLineByte(damagedSprite, (spriteIndex & 1) == 0 ? 2 : 0, data);    
  }
}


// convert vic cycle to sprite cycle, sprite coordinate 0 is at cycle 13-2 (both pal and ntsc)
// sprite cycle = 1 cycle every 8 pixels starting at sprite x coord 0
int32_t vic_getCurrentSpriteCycle() {
  int32_t spriteCoordinate = vic_cycle - 14;
  if (spriteCoordinate < 0) {
    spriteCoordinate += vic_CYCLES_PER_LINE;
  }
  return spriteCoordinate;
}


// calculate how many cycles from now a sprite should start being displayed
// on the current line and schedule a sprite event for that time
// one cycle = 8 pixels
void vic_handleSpriteVisibilityEvent(sprite_t *sprite) {
  int32_t xpos;
  int32_t count;

  // cancel the old sprite event if there was one
  clock_cancelEvent(&m64_clock, &(sprite->event));

  // if sprite x coordinate comes after the cycles have finished for the line
  // don't schedule the event
  if (sprite_getX(sprite) >> 3 >= vic_CYCLES_PER_LINE) {
    return;
  }

  // divide the sprite x position by 8
  xpos = (sprite_getX(sprite) >> 3);
  count = xpos - vic_getCurrentSpriteCycle();
  if (count <= 0) {
    count += vic_CYCLES_PER_LINE;
  }
  if (count > vic_CYCLES_PER_LINE) {
    count -= vic_CYCLES_PER_LINE;
  }

  // set where in the 8 pixels to start the display
  sprite_setDisplayStart(sprite, (sprite_getX(sprite) & 7));

  // schedule the sprite event
  clock_scheduleEvent(&m64_clock, &(sprite->event), count, PHASE_PHI2);
};


void vic_reset() {
  uint32_t i;

  vic_spriteLinkedListHead.nextVisibleSprite = NULL;

  for (i = 0; i < VIC_SPRITECOUNT; i++) {
    vic_sprites[i].consuming = false;
  }

  // set all pixels to black
  for (i = 0; i < VIC_PIXELS_LENGTH; ++i) {
    vic_pixels[i] = 0xff000000;
  }

  vic_graphicsRendering = false;
  memset((uint8_t *) vic_registers, 0, 0x40);

  // Video Matrix Counter
  vic_vc = 0;
  vic_vcBase = 0;

  // Row Counter
  vic_rc = 0;

  vic_isDisplayActive = false;
  vic_areBadLinesEnabled = false;
  vic_rasterY = 0;
  vic_phi1Data = 0;
  vic_showBorderVertical = true;
  vic_xscroll = 0;
  vic_yscroll = 0;
  vic_irqFlags = 0;
  vic_irqMask = 0;
  vic_nextPixel = 0;
  vic_lpx = 0;
  vic_lpy = 0;
  vic_determineVideoMemoryBaseAddresses();


  // reset based on model
  if(vic_model == VIC_MODEL6569) {
    // make sure 6567 is stopped
    m6567_stop();

    m6569_reset();
  }


  if(vic_model == VIC_MODEL6567R8) {
    // make sure 6569 is stopped
    m6569_stop();

    m6567_reset();
  }

}


void vic_triggerLightpen() {
  vic_lpAsserted = true;
  vic_lightpenEdgeDetector();
};

void vic_clearLightpen() {
  vic_lpAsserted = false;
};

//  if true, trigger the irq if not in it.
void vic_interrupt(bool_t b) {
  pla_setIRQ(b);
}

// set bus available, if ba is false, vic wants to use the bus
void vic_setBA(bool_t b) {
  pla_setBA(b);
};

uint8_t vic_vicReadColorMemoryPHI2(uint16_t address) {
  return pla_vicReadColorMemoryPHI2(address);
}

uint8_t vic_vicReadMemoryPHI1(uint16_t address) {
  return pla_vicReadMemoryPHI1(address);
}

uint8_t vic_vicReadMemoryPHI2(uint16_t address) {
  return pla_vicReadMemoryPHI2(address);
}
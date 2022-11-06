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

#ifndef SPRITE_H
#define SPRITE_H

struct sprite_s {
	event_t event;

	// sprite index
	uint32_t index;
	uint32_t indexBits;

	// maintain a list of sprites which are being displayed for the current cycle
	struct sprite_s *linkedListHead;
	struct sprite_s *nextVisibleSprite;

	// should the sprite be displayed
	bool_t display;

	// Is sprite pixel pipeline active?
	bool_t consuming;

	// This is the first read of multicolor pixel? (consuming read)
	bool_t firstMultiColorRead;

	// number of pixels to delay a sprite's appearance (0-7) in an 8 pixel block
	// is set to the lower 3 bits of sprite's x coordinate
	uint32_t offsetPixels;

	// 24 bits for a single line of sprite data (3 bytes)
	uint32_t lineData;
	// line data is copied into consumed line data and shifted by offsetPixels
	uint32_t consumedLineData;

	// pointer for the sprite
	// sprite address in memory = vic base address + pointerByte * 64
	uint8_t pointerByte;

	// offset into the sprite's data for the next byte to be read
	uint32_t mc;

	// offset into the sprite's data for the first byte of sprite data for the current raster line
	uint32_t mcBase;

	// is the sprite expanded vertically
	bool_t expandY;

	// if this is a y-expanded sprite, each line for the sprite is doubled
	// keep track if it's the first line in a doubled line
	bool_t firstYRead;

	// is the sprite expanded horizontally
	bool_t expandX;

	// if this is a x-exapnded sprite, each pixel is doubled horizontally
	// keep track if it's the first pixel in a doubled pixel
	bool_t firstXRead;

	// Sprite position
	int32_t x;
	int32_t y;

	// Is the sprite current enabled
	bool_t enabled;

	// Multicolor mode on? 
	bool_t multiColor;

	// Does the sprite have priority over the screen background?
	bool_t priorityOverForegroundGraphics;

	//The masking to be used for migrating color 1 as foreground color during sprite priority bit handling
	uint32_t priorityMask;

	bool_t multiColorLatched;
	bool_t expandXLatched;

	uint32_t prevPixel;

	// Allow display to be enabled.
	bool_t allowDisplay;

	// Sprite colors: 0, 1, and our own color. 
	uint32_t color[4];

	uint32_t prevPriority;

	// colors for 8 pixels
	// each pixel is 4 bits (0-15)
	uint32_t colorBuffer;

};

typedef struct sprite_s sprite_t;


void sprite_event(void *context);


void sprite_init(sprite_t *sprite, sprite_t *linkedListHead, uint32_t index);
void sprite_setDisplayStart(sprite_t *sprite, uint32_t offsetPixels);
int32_t sprite_getX(sprite_t *sprite);
void sprite_setX(sprite_t *sprite, int32_t x);
int32_t sprite_getY(sprite_t *sprite);
void sprite_setY(sprite_t *sprite, int32_t y);
void sprite_setPriorityOverForegroundGraphics(sprite_t *sprite, bool_t priority);
bool_t sprite_isEnabled(sprite_t *sprite);
void sprite_setEnabled(sprite_t *sprite, bool_t enabled);
void sprite_setExpandX(sprite_t *sprite, bool_t expandX);
void sprite_setExpandY(sprite_t *sprite, bool_t expandY, bool_t crunchCycle);
void sprite_setMulticolor(sprite_t *sprite, bool_t multiColor);
void sprite_beginDMA(sprite_t *sprite);
bool_t sprite_isDMA(sprite_t *sprite);
void sprite_setDisplay(sprite_t *sprite, bool_t display);
void sprite_setPointerByte(sprite_t *sprite, uint8_t pointerByte);
uint32_t sprite_getCurrentByteAddress(sprite_t *sprite);
void sprite_setLineByte(sprite_t *sprite, uint32_t idx, uint8_t value);
void sprite_initDmaAccess(sprite_t *sprite);
void sprite_finishDmaAccess(sprite_t *sprite);
void sprite_expandYFlipFlop(sprite_t *sprite);

void sprite_setColor(sprite_t *sprite, uint32_t idx, uint32_t val);
uint32_t sprite_calculateNext8Pixels(sprite_t *sprite);
void sprite_repeatPixels(sprite_t *sprite);
uint32_t sprite_getNextPriorityMask(sprite_t *sprite);
void sprite_setAllowDisplay(sprite_t *sprite, bool_t allowDisplay);



#endif
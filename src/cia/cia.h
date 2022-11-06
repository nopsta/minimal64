/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For the full
 * license text, see http://www.gnu.org/licenses/gpl.html.
 * 
 * Author: nopsta 2022
 * 
 * Based on parts of: 
 * MOS6526.java from JSIDPlay2: Antti S. Lankila (alankila@bel.fi)
 * SIDPLAY2 Copyright (©) 2001-2004 Simon White <sidplay2@yahoo.com>
 * VICE Copyright (©) 2009 VICE Project
 * 
 * See the cia/notes directory from more about the cia chip
 * 
 */

#ifndef CIA_H
#define CIA_H

extern m6526_t cia1;
extern m6526_t cia2;

void cia1_init(uint32_t model);

// write to a cia1 register
void cia1_write(uint16_t reg, uint8_t data);

// read from a cia1 register
uint8_t cia1_read(uint16_t reg);

void cia2_init(uint32_t model);

// write to a cia2 register
void cia2_write(uint16_t reg, uint8_t data);

// read from a cia2 register
uint8_t cia2_read(uint16_t reg);

#endif
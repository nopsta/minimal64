/*  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 *  Adapted from iecbus.c from VICE by nopsta 
 *  Original iecbus.c - written by Andreas Boose <viceteam@t-online.de>* 
 */

#ifndef IECBUS_H
#define IECBUS_H

#define IECBUS_NUM 16

void iecBus_init();
void iecBus_reset();

uint8_t iecBus_readFromIECBus();
void iecBus_writeToIECBus(uint8_t data);
void iecBus_updatePorts();

#endif
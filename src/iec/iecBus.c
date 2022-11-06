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


#include "../m64.h"

uint8_t iecBus_drvBus[IECBUS_NUM];
uint8_t iecBus_drvData[IECBUS_NUM];

uint8_t iecBus_drvPort;
uint8_t iecBus_cpuBus;
uint8_t iecBus_cpuPort;

uint32_t iecBus_driveCount;

uint32_t iecBus_serialDeviceCount;

void iecBus_init() {

  iecBus_drvPort = 0;
  iecBus_cpuBus = 0;
  iecBus_cpuPort = 0;

  iecBus_driveCount = 1;
  iecBus_serialDeviceCount = 0;
  iecBus_reset();

}

void iecBus_reset() {
  memset(iecBus_drvBus, 0xff, IECBUS_NUM);
  memset(iecBus_drvData, 0xff, IECBUS_NUM);

  iecBus_cpuBus = 0xff;
  iecBus_cpuPort = 0xff;
  iecBus_drvPort = 133;
}

uint8_t iecBus_readFromIECBus() {
  uint32_t i = 0;

  // clock all the serial devices..

  return iecBus_cpuPort;
}

void iecBus_writeToIECBus(uint8_t data) {
  uint32_t i = 0;

  // clock all the serial devices

  uint8_t oldCpuBus = iecBus_cpuBus;
  iecBus_cpuBus = (( ((data & 255) << 2 & 128) | ((data & 255) << 2 & 64) | ((data & 255) << 1 & 16) ) | 0);

  iecBus_updatePorts();
}

void iecBus_updatePorts() {
  iecBus_cpuPort = iecBus_cpuBus;
  iecBus_drvPort = ( ((iecBus_cpuPort & 255) >> 4 & 4) | (iecBus_cpuPort & 255) >> 7 | ((iecBus_cpuBus & 255) << 3 & 128) ) ;
}



#include "../m64.h"


#define KERNAL_ROM_LENGTH 0x2000

uint8_t KERNALROM[KERNAL_ROM_LENGTH];

bool_t kernal_isM64Kernal = true;

void m64_setKernalROM(uint8_t *data, uint32_t dataLength) {
  if(dataLength > KERNAL_ROM_LENGTH) {
    dataLength = KERNAL_ROM_LENGTH;
  }
  memcpy(KERNALROM, data, sizeof(uint8_t) * dataLength);
  kernal_isM64Kernal = false;
}

void kernal_init() {
  if(kernal_isM64Kernal) {
    uint32_t i;

    // a minimal custom m64 kernal:
    // checks for cartridge
    // sets processor port to $37
    // initialises registers
    // sets screen to black 
    // starts cia 1 timer to update jiffy clock at $a0
    // loops, waiting for a non zero value in $14, then jumps to address stored at $15/$16

    // prgs or crts that rely on c64 kernal or basic routines won't work

    // set the kernal/vectors
    // non-maskable interrupt service routine
    KERNALROM[0xfffa - 0xe000] = 0xc7;
    KERNALROM[0xfffb - 0xe000] = 0xe0;

    // cold reset vector
    KERNALROM[0xfffc - 0xe000] = 0x00;
    KERNALROM[0xfffd - 0xe000] = 0xe0;

    // interrupt service routine
    KERNALROM[0xfffe - 0xe000] = 0xc8;
    KERNALROM[0xffff - 0xe000] = 0xe0;

    i = 0;
    KERNALROM[i++] = 0x78; //e000
    KERNALROM[i++] = 0xa2; //e001
    KERNALROM[i++] = 0xff; //e002
    KERNALROM[i++] = 0x9a; //e003
    KERNALROM[i++] = 0xd8; //e004
    KERNALROM[i++] = 0xa2; //e005
    KERNALROM[i++] = 0x05; //e006
    KERNALROM[i++] = 0xbd; //e007
    KERNALROM[i++] = 0x03; //e008
    KERNALROM[i++] = 0x80; //e009
    // cart check code, point byte after dd should be auto set to one byte before cart id
    KERNALROM[i++] = 0xdd; //e00a
    KERNALROM[i++] = 0xf3; //e00b
    KERNALROM[i++] = 0xe0; //e00c
    KERNALROM[i++] = 0xd0; //e00d
    KERNALROM[i++] = 0x06; //e00e
    KERNALROM[i++] = 0xca; //e00f
    KERNALROM[i++] = 0xd0; //e010
    KERNALROM[i++] = 0xf5; //e011
    KERNALROM[i++] = 0x6c; //e012
    KERNALROM[i++] = 0x00; //e013
    KERNALROM[i++] = 0x80; //e014
    KERNALROM[i++] = 0xa9; //e015
    KERNALROM[i++] = 0x0b; //e016
    KERNALROM[i++] = 0x8d; //e017
    KERNALROM[i++] = 0x11; //e018
    KERNALROM[i++] = 0xd0; //e019
    KERNALROM[i++] = 0xa9; //e01a
    KERNALROM[i++] = 0x00; //e01b
    KERNALROM[i++] = 0x8d; //e01c
    KERNALROM[i++] = 0x18; //e01d
    KERNALROM[i++] = 0xd4; //e01e
    KERNALROM[i++] = 0xa9; //e01f
    KERNALROM[i++] = 0x7f; //e020
    KERNALROM[i++] = 0x8d; //e021
    KERNALROM[i++] = 0x0d; //e022
    KERNALROM[i++] = 0xdc; //e023
    KERNALROM[i++] = 0x8d; //e024
    KERNALROM[i++] = 0x0d; //e025
    KERNALROM[i++] = 0xdd; //e026
    KERNALROM[i++] = 0x8d; //e027
    KERNALROM[i++] = 0x00; //e028
    KERNALROM[i++] = 0xdc; //e029
    KERNALROM[i++] = 0xa9; //e02a
    KERNALROM[i++] = 0xff; //e02b
    KERNALROM[i++] = 0x8d; //e02c
    KERNALROM[i++] = 0x02; //e02d
    KERNALROM[i++] = 0xdc; //e02e
    KERNALROM[i++] = 0xa9; //e02f
    KERNALROM[i++] = 0x00; //e030
    KERNALROM[i++] = 0x8d; //e031
    KERNALROM[i++] = 0x03; //e032
    KERNALROM[i++] = 0xdc; //e033
    KERNALROM[i++] = 0x8d; //e034
    KERNALROM[i++] = 0x03; //e035
    KERNALROM[i++] = 0xdd; //e036
    KERNALROM[i++] = 0xa9; //e037
    KERNALROM[i++] = 0x3f; //e038
    KERNALROM[i++] = 0x8d; //e039
    KERNALROM[i++] = 0x02; //e03a
    KERNALROM[i++] = 0xdd; //e03b
    KERNALROM[i++] = 0xa9; //e03c
    KERNALROM[i++] = 0x07; //e03d
    KERNALROM[i++] = 0x8d; //e03e
    KERNALROM[i++] = 0x00; //e03f
    KERNALROM[i++] = 0xdd; //e040
    KERNALROM[i++] = 0xa9; //e041
    KERNALROM[i++] = 0x08; //e042
    KERNALROM[i++] = 0x8d; //e043
    KERNALROM[i++] = 0x0e; //e044
    KERNALROM[i++] = 0xdc; //e045
    KERNALROM[i++] = 0x8d; //e046
    KERNALROM[i++] = 0x0f; //e047
    KERNALROM[i++] = 0xdc; //e048
    KERNALROM[i++] = 0x8d; //e049
    KERNALROM[i++] = 0x0e; //e04a
    KERNALROM[i++] = 0xdd; //e04b
    KERNALROM[i++] = 0x8d; //e04c
    KERNALROM[i++] = 0x0f; //e04d
    KERNALROM[i++] = 0xdd; //e04e
    KERNALROM[i++] = 0xa9; //e04f
    KERNALROM[i++] = 0x37; //e050
    KERNALROM[i++] = 0x85; //e051
    KERNALROM[i++] = 0x01; //e052
    KERNALROM[i++] = 0xa2; //e053
    KERNALROM[i++] = 0x2f; //e054
    KERNALROM[i++] = 0x86; //e055
    KERNALROM[i++] = 0x00; //e056
    KERNALROM[i++] = 0xad; //e057
    KERNALROM[i++] = 0x12; //e058
    KERNALROM[i++] = 0xd0; //e059
    KERNALROM[i++] = 0xcd; //e05a
    KERNALROM[i++] = 0x12; //e05b
    KERNALROM[i++] = 0xd0; //e05c
    KERNALROM[i++] = 0xf0; //e05d
    KERNALROM[i++] = 0xfb; //e05e
    KERNALROM[i++] = 0x30; //e05f
    KERNALROM[i++] = 0xf6; //e060
    KERNALROM[i++] = 0xc9; //e061
    KERNALROM[i++] = 0x07; //e062
    KERNALROM[i++] = 0x90; //e063
    KERNALROM[i++] = 0x04; //e064
    KERNALROM[i++] = 0xa9; //e065
    KERNALROM[i++] = 0x01; //e066
    KERNALROM[i++] = 0xd0; //e067
    KERNALROM[i++] = 0x02; //e068
    KERNALROM[i++] = 0xa9; //e069
    KERNALROM[i++] = 0x00; //e06a
    KERNALROM[i++] = 0x8d; //e06b
    KERNALROM[i++] = 0xa6; //e06c
    KERNALROM[i++] = 0x02; //e06d
    KERNALROM[i++] = 0xaa; //e06e
    KERNALROM[i++] = 0xbd; //e06f
    // irq timing lo, should be auto set to irq timing table
    KERNALROM[i++] = 0xf9; //e070
    KERNALROM[i++] = 0xe0; //e071
    KERNALROM[i++] = 0x8d; //e072
    KERNALROM[i++] = 0x04; //e073
    KERNALROM[i++] = 0xdc; //e074
    KERNALROM[i++] = 0xbd; //e075
    // irq timing hi
    KERNALROM[i++] = 0xfb; //e076
    KERNALROM[i++] = 0xe0; //e077
    KERNALROM[i++] = 0x8d; //e078
    KERNALROM[i++] = 0x05; //e079
    KERNALROM[i++] = 0xdc; //e07a
    KERNALROM[i++] = 0xa0; //e07b
    KERNALROM[i++] = 0x00; //e07c
    KERNALROM[i++] = 0xa9; //e07d
    KERNALROM[i++] = 0x00; //e07e
    KERNALROM[i++] = 0x99; //e07f
    KERNALROM[i++] = 0x00; //e080
    KERNALROM[i++] = 0x03; //e081
    KERNALROM[i++] = 0x99; //e082
    KERNALROM[i++] = 0x00; //e083
    KERNALROM[i++] = 0x02; //e084
    KERNALROM[i++] = 0x99; //e085
    KERNALROM[i++] = 0x02; //e086
    KERNALROM[i++] = 0x00; //e087
    KERNALROM[i++] = 0xc8; //e088
    KERNALROM[i++] = 0xd0; //e089
    KERNALROM[i++] = 0xf4; //e08a
    KERNALROM[i++] = 0xa9; //e08b
    KERNALROM[i++] = 0x00; //e08c
    KERNALROM[i++] = 0xa2; //e08d
    KERNALROM[i++] = 0x2e; //e08e
    KERNALROM[i++] = 0x9d; //e08f
    KERNALROM[i++] = 0x00; //e090
    KERNALROM[i++] = 0xd0; //e091
    KERNALROM[i++] = 0xca; //e092
    KERNALROM[i++] = 0x10; //e093
    KERNALROM[i++] = 0xfa; //e094
    KERNALROM[i++] = 0x8d; //e095
    KERNALROM[i++] = 0x20; //e096
    KERNALROM[i++] = 0xd0; //e097
    KERNALROM[i++] = 0x8d; //e098
    KERNALROM[i++] = 0x21; //e099
    KERNALROM[i++] = 0xd0; //e09a
    KERNALROM[i++] = 0x85; //e09b
    KERNALROM[i++] = 0x14; //e09c
    KERNALROM[i++] = 0x85; //e09d
    KERNALROM[i++] = 0x15; //e09e
    KERNALROM[i++] = 0x85; //e09f
    KERNALROM[i++] = 0x16; //e0a0
    KERNALROM[i++] = 0xa9; //e0a1
    KERNALROM[i++] = 0x1b; //e0a2
    KERNALROM[i++] = 0x8d; //e0a3
    KERNALROM[i++] = 0x11; //e0a4
    KERNALROM[i++] = 0xd0; //e0a5
    KERNALROM[i++] = 0xa9; //e0a6
    KERNALROM[i++] = 0xc8; //e0a7
    KERNALROM[i++] = 0x8d; //e0a8
    KERNALROM[i++] = 0x16; //e0a9
    KERNALROM[i++] = 0xd0; //e0aa
    KERNALROM[i++] = 0xa9; //e0ab
    KERNALROM[i++] = 0x14; //e0ac
    KERNALROM[i++] = 0x8d; //e0ad
    KERNALROM[i++] = 0x18; //e0ae
    KERNALROM[i++] = 0xd0; //e0af
    KERNALROM[i++] = 0xa9; //e0b0
    KERNALROM[i++] = 0x0f; //e0b1
    KERNALROM[i++] = 0x8d; //e0b2
    KERNALROM[i++] = 0x19; //e0b3
    KERNALROM[i++] = 0xd0; //e0b4
    KERNALROM[i++] = 0xa9; //e0b5
    KERNALROM[i++] = 0x11; //e0b6
    KERNALROM[i++] = 0x8d; //e0b7
    KERNALROM[i++] = 0x0e; //e0b8
    KERNALROM[i++] = 0xdc; //e0b9
    KERNALROM[i++] = 0xa9; //e0ba
    KERNALROM[i++] = 0x81; //e0bb
    KERNALROM[i++] = 0x8d; //e0bc
    KERNALROM[i++] = 0x0d; //e0bd
    KERNALROM[i++] = 0xdc; //e0be
    KERNALROM[i++] = 0x58; //e0bf
    KERNALROM[i++] = 0xa5; //e0c0
    KERNALROM[i++] = 0x14; //e0c1
    // branch while waiting for a prg
    KERNALROM[i++] = 0xf0; //e0c2
    KERNALROM[i++] = 0xfc; //e0c3
    KERNALROM[i++] = 0x6c; //e0c4
    KERNALROM[i++] = 0x15; //e0c5
    KERNALROM[i++] = 0x00; //e0c6
    // nmi routine
    KERNALROM[i++] = 0x40; //e0c7
    // irq routine
    KERNALROM[i++] = 0x48; //e0c8
    KERNALROM[i++] = 0x8a; //e0c9
    KERNALROM[i++] = 0x48; //e0ca
    KERNALROM[i++] = 0x98; //e0cb
    KERNALROM[i++] = 0x48; //e0cc
    KERNALROM[i++] = 0xe6; //e0cd
    KERNALROM[i++] = 0xa2; //e0ce
    KERNALROM[i++] = 0xd0; //e0cf
    KERNALROM[i++] = 0x06; //e0d0
    KERNALROM[i++] = 0xe6; //e0d1
    KERNALROM[i++] = 0xa1; //e0d2
    KERNALROM[i++] = 0xd0; //e0d3
    KERNALROM[i++] = 0x02; //e0d4
    KERNALROM[i++] = 0xe6; //e0d5
    KERNALROM[i++] = 0xa0; //e0d6
    KERNALROM[i++] = 0xa5; //e0d7
    KERNALROM[i++] = 0xa0; //e0d8
    KERNALROM[i++] = 0xc9; //e0d9
    KERNALROM[i++] = 0x4f; //e0da
    KERNALROM[i++] = 0x90; //e0db
    KERNALROM[i++] = 0x0e; //e0dc
    KERNALROM[i++] = 0xa5; //e0dd
    KERNALROM[i++] = 0xa1; //e0de
    KERNALROM[i++] = 0xc9; //e0df
    KERNALROM[i++] = 0x1a; //e0e0
    KERNALROM[i++] = 0x90; //e0e1
    KERNALROM[i++] = 0x08; //e0e2
    KERNALROM[i++] = 0xa9; //e0e3
    KERNALROM[i++] = 0x00; //e0e4
    KERNALROM[i++] = 0x85; //e0e5
    KERNALROM[i++] = 0xa0; //e0e6
    KERNALROM[i++] = 0x85; //e0e7
    KERNALROM[i++] = 0xa1; //e0e8
    KERNALROM[i++] = 0x85; //e0e9
    KERNALROM[i++] = 0xa2; //e0ea
    KERNALROM[i++] = 0xad; //e0eb
    KERNALROM[i++] = 0x0d; //e0ec
    KERNALROM[i++] = 0xdc; //e0ed
    KERNALROM[i++] = 0x68; //e0ee
    KERNALROM[i++] = 0xa8; //e0ef
    KERNALROM[i++] = 0x68; //e0f0
    KERNALROM[i++] = 0xaa; //e0f1
    KERNALROM[i++] = 0x68; //e0f2
    KERNALROM[i++] = 0x40; //e0f3
    // cart id start
    KERNALROM[i++] = 0xc3; //e0f4
    KERNALROM[i++] = 0xc2; //e0f5
    KERNALROM[i++] = 0xcd; //e0f6
    KERNALROM[i++] = 0x38; //e0f7
    KERNALROM[i++] = 0x30; //e0f8
    KERNALROM[i++] = 0x95; //e0f9
    KERNALROM[i++] = 0x25; //e0fa
    KERNALROM[i++] = 0x42; //e0fb
    KERNALROM[i++] = 0x40; //e0fc



  }
}

bool_t kernal_getIsM64Kernal() {
  return kernal_isM64Kernal;
}

void kernal_reset() {
  // cant reset kernal rom
}

void kernal_write(uint16_t address, uint8_t value) {
  // cant write to kernal
}

uint8_t kernal_read(uint16_t address) {
  return KERNALROM[address & (KERNAL_ROM_LENGTH - 1)];
}

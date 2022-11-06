#include "m64.h"

int32_t m64_model;
m6510_t m64_cpu;
clock_t m64_clock;

#define PAL_CPU_FREQUENCY  985248
#define NTSC_CPU_FREQUENCY 1022727

// model 0 = NTSC, 1 = PAL
// sidModel 0 = 6581, 1 = 8580, 2 = 8580 with digiboost
void m64_init(int32_t model, int32_t sidModel) {

  if(model == M64_MODEL_NTSC) {
    m64_model = model;
  } else {
    m64_model = M64_MODEL_PAL;
  }

  m6510_buildInstructionTable();

  uint32_t mainsFrequency = 50;
  if(m64_model == M64_MODEL_NTSC) {
    mainsFrequency = 60;
    clock_init(&m64_clock, NTSC_CPU_FREQUENCY);
  } else {
    mainsFrequency = 50;    
    clock_init(&m64_clock, PAL_CPU_FREQUENCY);
  }


  m6510_setMemoryHandler(&m64_cpu, &pla_cpuRead, &pla_cpuWrite);
  m6510_init(&m64_cpu, &m64_clock);
  
  if(m64_model == M64_MODEL_PAL) {
    vic_init(VIC_MODEL6569);
  } else {
    vic_init(VIC_MODEL6567R8);
  }

  kernal_init();

  joystick_reset(&m64_joysticks[0]);
  joystick_reset(&m64_joysticks[1]);

  pla_init();
  pla_setCpu(&m64_cpu);

  iecBus_init();
  keyboard_init();

  cia1_init(M6526_MODEL_6526);
  cia2_init(M6526_MODEL_6526);

  m6526_setDayOfTimeRate(&cia1, clock_getCyclesPerSecond(&m64_clock) / mainsFrequency);
  m6526_setDayOfTimeRate(&cia2, clock_getCyclesPerSecond(&m64_clock) / mainsFrequency);

  cartridge_init();

  sid_init(SID_8580_DIGIBOOST, clock_getCyclesPerSecond(&m64_clock));

  m64_reset(0);
}



void m64_runUntilKernalReady() {

  // if no cartridge is inserted, run the kernal startup routine before returning
  // if a cartridge is inserted, the kernal startup will just run in normal time
  // running the start routine here mostly so it's safe to load/inject a prg anytime after calling m64_reset

  uint32_t pc = m64_cpu.nextOpcodeLocation;

  // run the machine until the program counter matches the end condition pc
  uint32_t endConditionPC = 0;
  uint32_t frames = 0;
  uint32_t i = 0;

  if(m64_cartridge.type == CARTRIDGE_NULL) {

    if(!kernal_getIsM64Kernal()) {
      // e5cd is where the kernal loops waiting for keyboard entry
      endConditionPC = 0xe5cd;
    } else {
      endConditionPC = 0xe0c2;
      
    }
    while(true) {
      // run one frame (or approx 1 frame)
      // pal has 63 (cycles per line) * 312 (lines)= 19656 cycles per frame, 
      // only need approximate here so should also work for ntsc (17095 cycles per frame)
      for(i = 0; i < 19656 * 2; i++) {  
        clock_step(&m64_clock);
        // check if the program counter is about to change
        if(m64_cpu.nextOpcodeLocation != pc) { 
          pc = m64_cpu.nextOpcodeLocation;
          // e5cd is where the kernal loops waiting for keyboard entry
          if(pc == endConditionPC) {
            return;
          }
        }
      }

      // an extra check to make sure don't loop forever
      frames++;
      if(frames > 200) {
//        printf("frames is %d, pc is %d\n", frames, pc);
        break;
      }
    }
  }  
}

// hard reset, soft reset not implemented
void m64_reset(uint32_t runUntilKernalIsReady) {

  clock_reset(&m64_clock);
  keyboard_reset();
  pla_reset();

  iecBus_reset();

  m6510_triggerRST(&m64_cpu);

  m6526_reset(&cia1);
  m6526_reset(&cia2);

  vic_reset();

  zeroram_reset();
  systemram_reset();
  colorram_reset();

  sid_reset();

  if(runUntilKernalIsReady) {
    m64_runUntilKernalReady();
  }
}


void m64_injectPrg(uint8_t *data, uint32_t dataLength) {
  uint32_t i;
  uint32_t address;
  uint32_t baseAddress = data[0];
  baseAddress += (data[1] << 8);

  uint8_t *ram = systemram_array();
  for(i = 0; i < dataLength - 2; i++) {
    address = baseAddress + i;
    ram[address] = data[i + 2];
  }

  if(!kernal_getIsM64Kernal()) {
    if(baseAddress == 0x801) {
      // set the beginning of basic variable area to end of program + 1
      address = baseAddress + dataLength - 1;
      ram[0x2d] = address & 0xff;
      ram[0x2e] = (address >> 8) & 0xff;

      ram[0x2f] = address & 0xff;
      ram[0x30] = (address >> 8) & 0xff;

      ram[0x31] = address & 0xff;
      ram[0x32] = (address >> 8) & 0xff;
    }
  }

}
// just write a prg direct to system ram
// address the prg is written is in the first 2 bytes
// load "*",8 loads to start of basic
// load "*",8,1 loads to address in first 2 bytes
void m64_injectAndRunPrg(uint8_t *data, uint32_t dataLength, uint32_t delay) {
  uint32_t i;
  uint32_t address;
  uint32_t baseAddress = data[0];
  baseAddress += (data[1] << 8);

  m64_reset(1);

  delay = delay * 2;
  for(i = 0; i < delay; i++) {
    clock_step(&m64_clock);
  }

  m64_injectPrg(data, dataLength);

  uint8_t *ram = systemram_array();

  // see if there is a basic sys command in the first 20 bytes
  // and get the address it jumps to
  // bytes 0-1: address where to load prg
  // bytes 2-3: next basic line
  // bytes 3-4: basic line number
  // search for the sys token (0x9e) after the basic line number
  int sysPosition = 0;
  int startAddressPosition = 0;
  int startAddress = 0;
  for(i = 4; i < 30; i++) {
    if(data[i] == 0x9e) {
      sysPosition = i;
    } else if(sysPosition != 0 && data[i] != 0x20) {
      // gone past the sys token and it's not a space
      if(startAddressPosition == 0) {
        startAddressPosition = i;
      }

      if(data[i] == 0) {
        // reached the terminating token
        break;
      }

      if(data[i] >= 0x30 && data[i] <= 0x39) {
        // it's a number token
        startAddress = startAddress * 10 + (data[i] - 0x30);
      }
    }
  }

  if(kernal_getIsM64Kernal()) {
    if(startAddress != 0) {
      // do the m64 kernal start prg
      pla_cpuWrite(0x15, startAddress & 0xff);
      pla_cpuWrite(0x16, (startAddress >> 8) & 0xff);
      pla_cpuWrite(0x14, 0x01);
    }
  } else {
    // maybe it's a kernal that supports 'run' ?

    if(baseAddress == 0x801) {
      // it's likely a basic program
      // put 'run:' into the buffer

      pla_cpuWrite(0xc6, 5); // buffer length

      pla_cpuWrite(0x277, 82);  // R
      pla_cpuWrite(0x278, 85);  // U
      pla_cpuWrite(0x279, 78);  // N
      pla_cpuWrite(0x27a, 58);  // :
      pla_cpuWrite(0x27a, 13);  // return
    }
  }
}

void m64_loadCartridge(uint8_t *data, uint32_t dataLength) {

  if(cartridge_read(data, dataLength) != 0) {
    m64_reset(0);
  }
}

void m64_removeCartridge() {
  // cartridge init sets it to a null cartridge
  cartridge_init();  
}

unsigned char *m64_getPixelBuffer() {
  uint32_t i;
  return (unsigned char *)vic_pixelBuffer;
}

uint32_t m64_getPixelBufferWidth() {
  return 48 * 8;
}

uint32_t m64_getPixelBufferHeight() {
  if(m64_model == M64_MODEL_NTSC) {
    return 252;
  }

  return 284;
}

int32_t m64_update(int32_t deltaTime) {

  int32_t screenDrawn = 0;
  int32_t screenDrawnInUpdate = 0;

  uint32_t i = 0;
  uint32_t j = 0;

  int64_t m64Frequency = 985248;

  if(m64_model == M64_MODEL_NTSC) {
    m64Frequency = 1022727;
  }

  int64_t endTime = (2 * m64Frequency * deltaTime * 100) / (1000 * 100);

  // make sure it doesnt get too big
  if(endTime > (m64Frequency * 2 * 1) / 25) {
    endTime = (m64Frequency * 2 * 1) / 25;
  }
  endTime += clock_getTimeAndPhase(&m64_clock);

  while(clock_getTimeAndPhase(&m64_clock) < endTime) {
    clock_step(&m64_clock);
    sid_update();
    
    if(vic_rasterY == 0 ) {
      if(!screenDrawn) {
        screenDrawn = 1;
        screenDrawnInUpdate = 1;

        // draw the screen
        memcpy(vic_pixelBuffer, vic_pixels, sizeof(uint32_t) * VIC_PIXELS_LENGTH);

      }
    } else {
      screenDrawn = 0;
    }
  }
  return screenDrawnInUpdate;
}

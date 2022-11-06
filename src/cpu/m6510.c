#include "m6510.h"
/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For the full
 * license text, see http://www.gnu.org/licenses/gpl.html.
 * 
 * Author: nopsta 2022
 * 
 * Based on MOS6510.java from JSidPlay2 by Ken Händel and Antti Lankila 
 * and mos6510c.i (C) 2000 by Simon White s_a_white@email.com
 * 
 * see cpu/notes for more on the 6510 cpu
 */


m6510_function m6510_instructionTable[INSTRTABLELENGTH];
bool_t m6510_processorCycleNoSteal[INSTRTABLELENGTH];

void m6510_setMemoryHandler(m6510_t *cpu, cpu_read_function read, cpu_write_function write) {
  cpu->cpuRead = read;
  cpu->cpuWrite = write;
}

/**
 * When AEC signal is high, no stealing is possible
 */
void eventWithoutSteals_function(void *context) {
  m6510_t *cpu = (m6510_t *)context;

  m6510_instructionTable[cpu->cycleCount++](cpu);

  clock_scheduleEvent(cpu->clock, &(cpu->eventWithoutSteals), 1, -1);
}

void eventWithSteals_function(void *context) {
  m6510_t *cpu = (m6510_t *)context;

  if(m6510_processorCycleNoSteal[cpu->cycleCount]) {
    m6510_instructionTable[cpu->cycleCount++](cpu);
    clock_scheduleEvent(cpu->clock, &(cpu->eventWithSteals), 1, -1);
  } else {
    if(cpu->interruptCycle == cpu->cycleCount) {
      cpu->interruptCycle--;
    }
  }

}


void m6510_initRegistersAndFlags(m6510_t *cpu) {
  cpu->registerSP = 0xff;//
  cpu->flagU = true;
  cpu->flagB = true;
  
  cpu->flagN = false; 
  cpu->flagC = false;
  cpu->flagD = false;
  cpu->flagV = false;
  cpu->flagZ = false;
  cpu->flagI = false;


  cpu->Register_ProgramCounter = 0;
  cpu->irqAssertedOnPin = false;
  cpu->nmiFlag = false;
  cpu->rstFlag = false;
  cpu->interruptCycle = M6510_MAX;
  cpu->rdy = true;
  clock_scheduleEvent(cpu->clock, &(cpu->eventWithoutSteals), 0, PHASE_PHI2);
}

void m6510_init(m6510_t *cpu, clock_t *clock) {
  cpu->clock = clock;
  
  cpu->eventWithSteals.event = &eventWithSteals_function;
  cpu->eventWithSteals.context = (void *)cpu;

  cpu->eventWithoutSteals.event = &eventWithoutSteals_function;
  cpu->eventWithoutSteals.context = (void *)cpu;
}

// Evaluate when to execute an interrupt. Calling this method can also
// result in the decision that no interrupt at all needs to be scheduled.
void  m6510_calculateInterruptTriggerCycle(m6510_t *cpu) {
  if (cpu->interruptCycle == M6510_MAX) {
    if (cpu->rstFlag || cpu->nmiFlag || (!cpu->flagI && cpu->irqAssertedOnPin)) {
      cpu->interruptCycle = cpu->cycleCount;
    }
  }
}



void m6510_interruptsAndNextOpcode(m6510_t *cpu) {
  
  if (cpu->cycleCount > cpu->interruptCycle + 2) {
    m6510_interrupt(cpu);
  } else {
    m6510_fetchNextOpcode(cpu);
  }
}

void m6510_interrupt(m6510_t *cpu) {
  (cpu->cpuRead)(cpu->Register_ProgramCounter);
  cpu->cycleCount = IOpCode_BRKn << 3;
  cpu->flagB = false;
  cpu->interruptCycle = M6510_MAX;
}

void m6510_interruptEnd(m6510_t *cpu) {
  cpu->Register_ProgramCounter = cpu->Cycle_EffectiveAddress;
  m6510_interruptsAndNextOpcode(cpu);
}

void m6510_setPC(m6510_t *cpu, uint16_t address) {
  cpu->gotoAddress = address;
}

void m6510_fetchNextOpcode(m6510_t *cpu) {
  if(cpu->gotoAddress != false) {
    cpu->Register_ProgramCounter = cpu->gotoAddress;
    cpu->gotoAddress = false;
  } 

  cpu->lastCycleCountAddress =  cpu->nextOpcodeLocation;
  // store the cycles of the last instruction, with hack to account for extra cycles if branch taken
  cpu->lastCycleCount = (cpu->cycleCount & 0x7) + cpu->branchCycleCount;
  cpu->branchCycleCount = 0;


  cpu->nextOpcodeLocation = cpu->Register_ProgramCounter;
  // cycle count is the current instruction and the subcycle in the instruction
  // set cycle count to the first cycle of the next instruction
  cpu->cycleCount = ((cpu->cpuRead)(cpu->Register_ProgramCounter) & 0xff) << 3;


  cpu->Register_ProgramCounter = (cpu->Register_ProgramCounter + 1) & 0xffff;

  if (!cpu->rstFlag && !cpu->nmiFlag && !(!cpu->flagI && cpu->irqAssertedOnPin)) {
    cpu->interruptCycle = M6510_MAX;
  }

  if (cpu->interruptCycle != M6510_MAX) {
    cpu->interruptCycle = -M6510_MAX;
  } 
}

// Fetch low address byte, increment PC
void m6510_FetchLowAddr(m6510_t *cpu) {
  cpu->Cycle_EffectiveAddress = (cpu->cpuRead)(cpu->Register_ProgramCounter) & 0xff;
  cpu->Register_ProgramCounter = (cpu->Register_ProgramCounter + 1) & 0xffff;
}

// Read from address, add index register X to it
void m6510_FetchLowAddrX(m6510_t *cpu) {
  m6510_FetchLowAddr(cpu);
  cpu->Cycle_EffectiveAddress = cpu->Cycle_EffectiveAddress + cpu->registerX & 255;
}

// Read from address, add index register Y to it
void m6510_FetchLowAddrY(m6510_t *cpu) {
  m6510_FetchLowAddr(cpu);
  cpu->Cycle_EffectiveAddress = cpu->Cycle_EffectiveAddress + cpu->registerY & 255;
}

// Fetch high address byte, increment PC (Absolute Addressing)
void m6510_FetchHighAddr(m6510_t *cpu) {
  cpu->Cycle_EffectiveAddress |= ((cpu->cpuRead)(cpu->Register_ProgramCounter) & 0xff) << 8;
  cpu->Register_ProgramCounter = (cpu->Register_ProgramCounter + 1) & 0xffff;
}

// Fetch high byte of address, add index register X to low address byte
void m6510_FetchHighAddrX(m6510_t *cpu) {
  m6510_FetchHighAddr(cpu);
  cpu->Cycle_HighByteWrongEffectiveAddress = (cpu->Cycle_EffectiveAddress & 0xff00) | (cpu->Cycle_EffectiveAddress + cpu->registerX & 0xff);
  cpu->Cycle_EffectiveAddress = cpu->Cycle_EffectiveAddress + (cpu->registerX & 0xff) & 0xffff;
}

// Fetch high byte of address, add index register Y to low address byte
void m6510_FetchHighAddrY(m6510_t *cpu) {
  m6510_FetchHighAddr(cpu);
  cpu->Cycle_HighByteWrongEffectiveAddress = (cpu->Cycle_EffectiveAddress & 0xff00) | (cpu->Cycle_EffectiveAddress + cpu->registerY & 0xff);
  cpu->Cycle_EffectiveAddress = cpu->Cycle_EffectiveAddress + (cpu->registerY & 0xff) & 0xffff;
}

// Fetch effective address low
void m6510_FetchLowEffAddr(m6510_t *cpu) {
  cpu->Cycle_EffectiveAddress = (cpu->cpuRead)(cpu->Cycle_Pointer) & 0xff;
}

// Fetch effective address high
void m6510_FetchHighEffAddr(m6510_t *cpu) {
  cpu->Cycle_Pointer = (cpu->Cycle_Pointer & 0xff00) | (cpu->Cycle_Pointer + 1 & 0xff);
  cpu->Cycle_EffectiveAddress |= ( (cpu->cpuRead)(cpu->Cycle_Pointer) & 0xff) << 8;
}

// Fetch effective address high, add Y to low byte of effective address
void m6510_FetchHighEffAddrY(m6510_t *cpu) {
  m6510_FetchHighEffAddr(cpu);
  cpu->Cycle_HighByteWrongEffectiveAddress = (cpu->Cycle_EffectiveAddress & 65280) | (cpu->Cycle_EffectiveAddress + cpu->registerY & 255);
  cpu->Cycle_EffectiveAddress = cpu->Cycle_EffectiveAddress + (cpu->registerY & 255) & 65535;
}

// Fetch pointer address low, increment PC
void m6510_FetchLowPointer(m6510_t *cpu) {
  cpu->Cycle_Pointer = (cpu->cpuRead)(cpu->Register_ProgramCounter) & 255;
  cpu->Register_ProgramCounter = cpu->Register_ProgramCounter + 1 & 65535;
}

// Fetch pointer address high, increment PC<BR>
void m6510_FetchHighPointer(m6510_t *cpu) {
  cpu->Cycle_Pointer |= ( (cpu->cpuRead)(cpu->Register_ProgramCounter) & 255) << 8;
  cpu->Register_ProgramCounter = cpu->Register_ProgramCounter + 1 & 65535;
}

// Write Cycle_Data to effective address.
void m6510_PutEffAddrDataByte(m6510_t *cpu) {
  (cpu->cpuWrite)(cpu->Cycle_EffectiveAddress, cpu->cycleData);
}

// Push Program Counter Low Byte on stack, decrement S
void m6510_PushLowPC(m6510_t *cpu) {
  (cpu->cpuWrite)(M6510_SP_PAGE << 8 | (cpu->registerSP & 255), ((cpu->Register_ProgramCounter & 255) | 0));
  cpu->registerSP--;
}

// Push Program Counter High Byte on stack, decrement S
void m6510_PushHighPC(m6510_t *cpu) {
  (cpu->cpuWrite)(M6510_SP_PAGE << 8 | (cpu->registerSP & 255), ((cpu->Register_ProgramCounter >> 8) | 0));
  cpu->registerSP--;
}

// Push P on stack, decrement S
void m6510_PushSR(m6510_t *cpu) {
  (cpu->cpuWrite)(M6510_SP_PAGE << 8 | (cpu->registerSP & 255), m6510_getStatusRegister(cpu));
  cpu->registerSP--;
}

// Increment stack and pull program counter low byte from stack,
void m6510_PopLowPC(m6510_t *cpu) {
  cpu->registerSP++;
  cpu->Cycle_EffectiveAddress = (cpu->cpuRead)(M6510_SP_PAGE << 8 | (cpu->registerSP & 255) ) & 255;

}

// Increment stack and pull program counter high byte from stack,
void m6510_PopHighPC(m6510_t *cpu) {
  cpu->registerSP++;
  cpu->Cycle_EffectiveAddress |= ((cpu->cpuRead)(M6510_SP_PAGE << 8 | (cpu->registerSP & 255) ) & 255) << 8;

}

// increment S, Pop P off stack
void m6510_PopSR(m6510_t *cpu) {
  cpu->registerSP++;
  m6510_setStatusRegister(cpu, (cpu->cpuRead)(M6510_SP_PAGE << 8 | (cpu->registerSP & 255) ));
  cpu->flagB = cpu->flagU = true;
}

// BCD adding
void m6510_doADC(m6510_t *cpu) {
    
  int32_t C = cpu->flagC ? 1 : 0;
  int32_t A = cpu->registerA & 0xff;
  int32_t s = cpu->cycleData & 0xff;
  
  int32_t regAC2 = A + s + C;

  int32_t lo;
  int32_t hi;

  if (cpu->flagD) {
      
    lo = (A & 15) + (s & 15) + C;
    hi = (A & 240) + (s & 240);
    if (lo > 9) {
        lo += 6;
    }
    if (lo > 15) {
        hi += 16;
    }
    cpu->flagZ = (regAC2 & 255) == 0;
    cpu->flagN = (hi & 128) != 0;
//    cpu->flagV = (cpu->v)(((hi ^ A) & 128) != 0 && ((A ^ s) & 128) == 0);
    cpu->flagV = ((hi ^ A) & 128) != 0 && ((A ^ s) & 128) == 0;
    if (hi > 144) {
        hi += 96;
    }
    cpu->flagC = hi > 255;
    cpu->registerA = ((  (hi & 240) | (lo & 15) ) & 0xff);
  }
  else {
    cpu->flagC = regAC2 > 255;
//    cpu->flagV = (cpu->v)(((regAC2 ^ A) & 128) != 0 && ((A ^ s) & 128) == 0);
    cpu->flagV = ((regAC2 ^ A) & 128) != 0 && ((A ^ s) & 128) == 0;

    cpu->registerA = regAC2 & 0xff;
    m6510_setFlagsNZ(cpu, cpu->registerA);
  }
}

// BCD subtracting
void m6510_doSBC(m6510_t *cpu) {
  // carry
  int32_t C = cpu->flagC ? 0 : 1;
  // accumulator
  int32_t A = cpu->registerA & 0xff;

  // current value
  int32_t s = cpu->cycleData & 0xff;

  int32_t regAC2 = ((A - s - C) >> 0) & 0xffff;



  // carry
  cpu->flagC = (regAC2 < 0x100);//regAC2 >= 0;

  // overflow
//  cpu->flagV = (cpu->v)(((regAC2 ^ A) & 128) != 0 && ((A ^ s) & 128) != 0);
  cpu->flagV = ((regAC2 ^ A) & 128) != 0 && ((A ^ s) & 128) != 0;

  // negative/zero
  m6510_setFlagsNZ(cpu, regAC2 & 0xff);
  if (cpu->flagD) {

    // decimal?

    int32_t lo = (A & 15) - (s & 15) - C;
    int32_t hi = (A & 240) - (s & 240);
    if ((lo & 16) != 0) {
        lo -= 6;
        hi -= 16;
    }
    if ((hi & 256) != 0) {
        hi -= 96;
    }
    cpu->registerA = (( (hi & 240) | (lo & 15) ) & 0xff);
  }
  else {
    cpu->registerA = regAC2  & 0xff;
  }
}

/*
// Force CPU to start execution at given address
void m6510_forcedJump(m6510_t *cpu, uint16_t address) {
  cpu->cycleCount = (IOpCode_NOPn << 3) + 1;
  cpu->Cycle_EffectiveAddress = cpu->Register_ProgramCounter = address;
}
*/

// Handle bus access signal. When RDY line is asserted, the CPU will pause
// when executing the next read operation.
void m6510_setRDY(m6510_t *cpu, bool_t rdy) {
  cpu->rdy = rdy;
  if (rdy) {
    clock_cancelEvent(cpu->clock, &(cpu->eventWithSteals));
    clock_scheduleEvent(cpu->clock, &(cpu->eventWithoutSteals), 0, PHASE_PHI2);
  } else {
    clock_cancelEvent(cpu->clock, &(cpu->eventWithoutSteals));
    clock_scheduleEvent(cpu->clock, &(cpu->eventWithSteals), 0, PHASE_PHI2);
  }
}


/**
 * This forces the CPU to abort whatever it is doing and immediately enter
 * the RST interrupt handling sequence. The implementation is not
 * compatible: instructions actually get aborted mid-execution. However,
 * there is no possible way to trigger this signal from programs, so it's
 * OK.
 */
void m6510_triggerRST(m6510_t *cpu) {

  m6510_initRegistersAndFlags(cpu);

  cpu->cycleCount = (IOpCode_BRKn << 3);
  cpu->rstFlag = true;
  m6510_calculateInterruptTriggerCycle(cpu);
}

/**
 * Trigger NMI interrupt on the CPU. Calling this method flags that CPU must
 * enter the NMI routine at earliest opportunity. There is no way to cancel
 * NMI request once given.
 */
void m6510_triggerNMI(m6510_t *cpu) {
  cpu->nmiFlag = true;
  m6510_calculateInterruptTriggerCycle(cpu);
  if (!cpu->rdy) {
    clock_cancelEvent(cpu->clock, &(cpu->eventWithSteals));
    clock_scheduleEvent(cpu->clock, &(cpu->eventWithSteals), 0, PHASE_PHI2);
  }

}

void m6510_triggerIRQ(m6510_t *cpu) {
  cpu->irqAssertedOnPin = true;
  m6510_calculateInterruptTriggerCycle(cpu);
  if (!cpu->rdy && cpu->interruptCycle == cpu->cycleCount) {
    clock_cancelEvent(cpu->clock, &(cpu->eventWithSteals));
    clock_scheduleEvent(cpu->clock, &(cpu->eventWithSteals), 0, PHASE_PHI2);
  }
}

// Inform CPU that IRQ is no longer pulled low.
void m6510_clearIRQ(m6510_t *cpu) {
  cpu->irqAssertedOnPin = false;
  m6510_calculateInterruptTriggerCycle(cpu);
}



// Set N and Z flag values.
void m6510_setFlagsNZ(m6510_t *cpu, uint8_t value) {
  cpu->flagZ = value == 0;
  cpu->flagN = (value < 0) || ((value & 0x80) > 0);
}

uint8_t m6510_getStatusRegister(m6510_t *cpu) {
  uint8_t sr = 0;
  if (cpu->flagN) {
      sr |= 128;
  }
//  (cpu->v)(cpu->flagV);
  if (cpu->flagV) {
      sr |= 64;
  }
  if (cpu->flagU) {
      sr |= 32;
  }
  if (cpu->flagB) {
      sr |= 16;
  }
  if (cpu->flagD) {
      sr |= 8;
  }
  if (cpu->flagI) {
      sr |= 4;
  }
  if (cpu->flagZ) {
      sr |= 2;
  }
  if (cpu->flagC) {
      sr |= 1;
  }
  return sr;
}

void m6510_setStatusRegister(m6510_t *cpu, uint8_t sr) {
  cpu->flagC = (sr & 1) != 0;
  cpu->flagZ = (sr & 2) != 0;
  cpu->flagI = (sr & 4) != 0;
  cpu->flagD = (sr & 8) != 0;
  cpu->flagB = (sr & 16) != 0;
  cpu->flagU = (sr & 32) != 0;
//  cpu->flagV = (cpu->v)((sr & 64) != 0);
  cpu->flagV = (sr & 64) != 0;
  cpu->flagN = (sr & 128) != 0;
}

/**
 * When stalled by BA but not yet tristated by AEC, the CPU generates read
 * requests to the PLA chip. These reads likely concern whatever byte the
 * CPU's current subcycle would need, but full emulation can be really
 * tricky. We normally have this case only immediately after a write opcode,
 * and thus the next read will concern the next opcode. Therefore, we fake
 * it by reading the byte under the PC.
 *
 */
uint8_t m6510_getStalledOnByte(m6510_t *cpu) {
  return (cpu->cpuRead)(cpu->Register_ProgramCounter);
}

/*********** INSTRUCTION TABLE ***************/
void m6510_wastedStealable(m6510_t *cpu) {
}

void throwAwayReadStealable(m6510_t *cpu) { 
  (cpu->cpuRead)(cpu->Cycle_HighByteWrongEffectiveAddress); 
}

void m6510_writeToEffectiveAddress(m6510_t *cpu) { 
  m6510_PutEffAddrDataByte(cpu); 
}

void m6510_readProgramCounter(m6510_t *cpu) {
  (cpu->cpuRead)(cpu->Register_ProgramCounter);
}

void m6510_instr2(m6510_t *cpu) {
  // read next value into Cycle_Data
  cpu->cycleData = (cpu->cpuRead)(cpu->Register_ProgramCounter);
  if (cpu->flagB) {
    cpu->Register_ProgramCounter = (cpu->Register_ProgramCounter + 1) & 0xffff;
  }
}

void m6510_instr7(m6510_t *cpu) {
  m6510_FetchHighAddrX(cpu);
  if (cpu->Cycle_EffectiveAddress == cpu->Cycle_HighByteWrongEffectiveAddress) {
    cpu->cycleCount++;
  }
}


void m6510_instr9(m6510_t *cpu) {
  m6510_FetchHighAddrY(cpu);
  if (cpu->Cycle_EffectiveAddress == cpu->Cycle_HighByteWrongEffectiveAddress) {
    cpu->cycleCount++;
  }
}

void m6510_instr10(m6510_t *cpu) {
  m6510_FetchHighAddrY(cpu);
  if (cpu->Cycle_EffectiveAddress == cpu->Cycle_HighByteWrongEffectiveAddress) {
    cpu->cycleCount++;
  }
}


void m6510_instr16(m6510_t *cpu) {
  cpu->Cycle_Pointer = cpu->Cycle_Pointer + cpu->registerX & 0xff;
}

void m6510_instr17(m6510_t *cpu) {
  m6510_FetchHighEffAddrY(cpu);
  if (cpu->Cycle_EffectiveAddress == cpu->Cycle_HighByteWrongEffectiveAddress) {
    cpu->cycleCount++;
  } 
}

void m6510_instrReadCycleData(m6510_t *cpu) {
  cpu->cycleData = (cpu->cpuRead)(cpu->Cycle_EffectiveAddress);   
}




void m6510_instr19(m6510_t *cpu) {
  m6510_doADC(cpu);
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr20(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, cpu->registerA &= cpu->cycleData);
  cpu->flagC = cpu->flagN;
  m6510_interruptsAndNextOpcode(cpu);
}            
void m6510_instr21(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, cpu->registerA &= cpu->cycleData);
  m6510_interruptsAndNextOpcode(cpu);
}


void m6510_instr22(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, cpu->registerA = (((cpu->registerA | M6510_ANE_CONST) & cpu->registerX & cpu->cycleData) & 0xff));
  m6510_interruptsAndNextOpcode(cpu);
}

void m6510_instr23(m6510_t *cpu) {
  uint8_t data = cpu->cycleData & cpu->registerA & 0xff;
  cpu->registerA = ((data >> 1) & 0xff);
  if (cpu->flagC) {
    cpu->registerA |= 128;
  }
  if (cpu->flagD) {
      cpu->flagN = cpu->flagC;
      cpu->flagZ = cpu->registerA == 0;
      cpu->flagV = ((data ^ cpu->registerA) & 64) != 0;
      if ((data & 15) + (data & 1) > 5) {
          cpu->registerA = (( (cpu->registerA & 240) | (cpu->registerA + 6 & 15) ) & 0xff);
      }
      cpu->flagC = (data + (data & 16) & 496) > 80;
      if (cpu->flagC) {
          cpu->registerA = ((cpu->registerA + 96) & 0xff);
      }
  }
  else {
      m6510_setFlagsNZ(cpu, cpu->registerA);
      cpu->flagC = (cpu->registerA & 64) != 0;
      cpu->flagV = ( (cpu->registerA & 64) ^ (cpu->registerA & 32) << 1) != 0;
  }
  m6510_interruptsAndNextOpcode(cpu);
}  

void m6510_instr24(m6510_t *cpu) {
  cpu->flagC = (cpu->registerA & 0x80) > 0;
  cpu->registerA <<= 1;
  m6510_setFlagsNZ(cpu, cpu->registerA);
  m6510_interruptsAndNextOpcode(cpu);
}

void m6510_instr25(m6510_t *cpu) {
  m6510_PutEffAddrDataByte(cpu);
  cpu->flagC = (cpu->cycleData & 0x80) > 0;// < 0;
  cpu->cycleData = (cpu->cycleData << 1) & 0xff;
  m6510_setFlagsNZ(cpu, cpu->cycleData);
}  

void m6510_instr26(m6510_t *cpu) {
  cpu->registerA &= cpu->cycleData;
  cpu->flagC = (cpu->registerA & 1) != 0;
  cpu->registerA >>= 1;
  cpu->registerA &= 127;
  m6510_setFlagsNZ(cpu, cpu->registerA);
  m6510_interruptsAndNextOpcode(cpu);
}


void m6510_instr27(m6510_t *cpu) {
    cpu->flagZ = (cpu->registerA & cpu->cycleData) == 0;
    cpu->flagN = (cpu->cycleData & 0x80) > 0;//< 0;
    cpu->flagV = (cpu->cycleData & 64) != 0;
    m6510_interruptsAndNextOpcode(cpu);
}

void m6510_instr29(m6510_t *cpu) {
  m6510_PushLowPC(cpu);
  if (cpu->rstFlag) {
      cpu->Cycle_EffectiveAddress = 0xfffc;
  } else if (cpu->nmiFlag) {
      // nmi = %01x 
      cpu->Cycle_EffectiveAddress = 0xfffa;
  } else {
    // irq = %11x 
      cpu->Cycle_EffectiveAddress = 0xfffe;
  }
  cpu->rstFlag = false;
  cpu->nmiFlag = false;
  m6510_calculateInterruptTriggerCycle(cpu);
}

void m6510_instr30(m6510_t *cpu) {
  m6510_PushSR(cpu);
  cpu->flagB = true;
  cpu->flagI = true;
}

void m6510_instr31(m6510_t *cpu) {
  cpu->Register_ProgramCounter = (cpu->cpuRead)(cpu->Cycle_EffectiveAddress) & 0xff; 
}
void m6510_instr32(m6510_t *cpu) {
  cpu->Register_ProgramCounter |= ((cpu->cpuRead)(cpu->Cycle_EffectiveAddress + 1) & 0xff) << 8; 

};
void m6510_instr34(m6510_t *cpu) {

  cpu->flagC = false;
  m6510_interruptsAndNextOpcode(cpu);
}

void m6510_instr35(m6510_t *cpu) {
  cpu->flagD = false;
  m6510_interruptsAndNextOpcode(cpu);
}

void m6510_instr36(m6510_t *cpu) {
    cpu->flagI = false;
    m6510_calculateInterruptTriggerCycle(cpu);
    m6510_interruptsAndNextOpcode(cpu);
}

void m6510_instr37(m6510_t *cpu) {
//  cpu->flagV = (cpu->v)(false);
  cpu->flagV = false;
  m6510_interruptsAndNextOpcode(cpu);
}

void m6510_instr38(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, ((cpu->registerA - cpu->cycleData) & 0xff));
  cpu->flagC = (cpu->registerA & 0xff) >= (cpu->cycleData & 0xff);
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr39(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, ((cpu->registerX - cpu->cycleData) & 0xff));
  cpu->flagC = (cpu->registerX & 0xff) >= (cpu->cycleData & 0xff);
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr40(m6510_t *cpu) {
  
  m6510_setFlagsNZ(cpu, ((cpu->registerY - cpu->cycleData) & 0xff));
  cpu->flagC = (cpu->registerY & 0xff) >= (cpu->cycleData & 0xff);
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr41(m6510_t *cpu) {
  m6510_PutEffAddrDataByte(cpu);
  cpu->cycleData--;


  m6510_setFlagsNZ(cpu, ((cpu->registerA - cpu->cycleData) & 0xff));
  cpu->flagC = (cpu->registerA & 0xff) >= (cpu->cycleData & 0xff);
}

void m6510_instr42(m6510_t *cpu) {
  m6510_PutEffAddrDataByte(cpu);
  cpu->cycleData = (cpu->cycleData - 1) & 0xff;
  m6510_setFlagsNZ(cpu, cpu->cycleData);
};

void m6510_instr43(m6510_t *cpu) {
  cpu->registerX = (cpu->registerX - 1) & 0xff;
  m6510_setFlagsNZ(cpu, cpu->registerX);
  m6510_interruptsAndNextOpcode(cpu);
}

void m6510_instr44(m6510_t *cpu) {
  cpu->registerY = (cpu->registerY - 1) & 0xff;
  m6510_setFlagsNZ(cpu, cpu->registerY);
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr45(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, cpu->registerA ^= cpu->cycleData);
  m6510_interruptsAndNextOpcode(cpu);
}

void m6510_instr46(m6510_t *cpu) {
  m6510_PutEffAddrDataByte(cpu);
  cpu->cycleData = (cpu->cycleData + 1) & 0xff;
  m6510_setFlagsNZ(cpu, cpu->cycleData);

}

void m6510_instr47(m6510_t *cpu) {
  cpu->registerX = (cpu->registerX + 1) & 0xff;
  m6510_setFlagsNZ(cpu, cpu->registerX);
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr48(m6510_t *cpu) {

  cpu->registerY = (cpu->registerY + 1) & 0xff;
  m6510_setFlagsNZ(cpu, cpu->registerY);
  m6510_interruptsAndNextOpcode(cpu);
};
void m6510_instr49(m6510_t *cpu) {
  m6510_PutEffAddrDataByte(cpu);
  cpu->cycleData = (cpu->cycleData + 1) & 0xff;
  m6510_setFlagsNZ(cpu, cpu->cycleData);


  m6510_doSBC(cpu);
};


void m6510_instr53(m6510_t *cpu) {
  cpu->Register_ProgramCounter = cpu->Cycle_EffectiveAddress;
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr54(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, cpu->cycleData &= cpu->registerSP);


  cpu->registerA = cpu->cycleData;
  cpu->registerX = cpu->cycleData;
  cpu->registerSP = cpu->cycleData;
  m6510_interruptsAndNextOpcode(cpu);
};

void m6510_instr55(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, cpu->registerA = cpu->registerX = cpu->cycleData);
  m6510_interruptsAndNextOpcode(cpu);
};
void m6510_instr56(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, cpu->registerA = cpu->cycleData);
  m6510_interruptsAndNextOpcode(cpu);
};
void m6510_instr57(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, cpu->registerX = cpu->cycleData);
  m6510_interruptsAndNextOpcode(cpu);
};
void m6510_instr58(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, cpu->registerY = cpu->cycleData);
  m6510_interruptsAndNextOpcode(cpu);
};
void m6510_instr59(m6510_t *cpu) {
  cpu->flagC = (cpu->registerA & 1) != 0;
  cpu->registerA >>= 1;
  cpu->registerA &= 127;
  m6510_setFlagsNZ(cpu, cpu->registerA);
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr60(m6510_t *cpu) {
  m6510_PutEffAddrDataByte(cpu);
  cpu->flagC = (cpu->cycleData & 1) != 0;
  cpu->cycleData >>= 1;
  cpu->cycleData &= 127;
  m6510_setFlagsNZ(cpu, cpu->cycleData);
}
void m6510_instr61(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, cpu->registerX = cpu->registerA = ((cpu->cycleData & (cpu->registerA | 238)) &0xff));
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr62(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, cpu->registerA |= cpu->cycleData);
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr63(m6510_t *cpu) {
  (cpu->cpuWrite)(M6510_SP_PAGE << 8 | (cpu->registerSP & 0xff), cpu->registerA);
  cpu->registerSP--;
};


void m6510_instr65(m6510_t *cpu) {
  cpu->registerSP++;
  m6510_setFlagsNZ(cpu, cpu->registerA = (cpu->cpuRead)(M6510_SP_PAGE << 8 | (cpu->registerSP & 255)));
}
void m6510_instr66(m6510_t *cpu) {
  m6510_PopSR(cpu);
  m6510_calculateInterruptTriggerCycle(cpu);
}

void m6510_instr68(m6510_t *cpu) {
  bool_t newC = (cpu->cycleData & 0x80) > 0; //< 0;
  m6510_PutEffAddrDataByte(cpu);
  cpu->cycleData <<= 1;
  if (cpu->flagC) {
      cpu->cycleData |= 1;
  }

  cpu->flagC = newC;
  m6510_setFlagsNZ(cpu, cpu->registerA &= cpu->cycleData);
};
void m6510_instr69(m6510_t *cpu) {
  bool_t newC = (cpu->registerA & 0x80) > 0;
  cpu->registerA <<= 1;
  if (cpu->flagC) {
      cpu->registerA |= 1;
  }
  m6510_setFlagsNZ(cpu, cpu->registerA);
  cpu->flagC = newC;
  m6510_interruptsAndNextOpcode(cpu);
};

void m6510_instr70(m6510_t *cpu) {
  bool_t newC = (cpu->cycleData & 0x80) > 0;//< 0;
  m6510_PutEffAddrDataByte(cpu);
  cpu->cycleData <<= 1;
  if (cpu->flagC) {
      cpu->cycleData |= 1;
  }
  m6510_setFlagsNZ(cpu, cpu->cycleData);
  cpu->flagC = newC;
};
void m6510_instr71(m6510_t *cpu) {
  bool_t newC = (cpu->registerA & 1) != 0;
  cpu->registerA >>= 1;
  if (cpu->flagC) {
      cpu->registerA |= 128;
  } else {
      cpu->registerA &= 127;
  }
  m6510_setFlagsNZ(cpu, cpu->registerA);
  cpu->flagC = newC;
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr72(m6510_t *cpu) {
  bool_t newC = (cpu->cycleData & 1) != 0;
  m6510_PutEffAddrDataByte(cpu);
  cpu->cycleData >>= 1;
  if (cpu->flagC) {
      cpu->cycleData |= 128;
  } else {
      cpu->cycleData &= 127;
  }
  m6510_setFlagsNZ(cpu, cpu->cycleData);
  cpu->flagC = newC;
};
void m6510_instr73(m6510_t *cpu) {
  bool_t newC = (cpu->cycleData & 1) != 0;
  m6510_PutEffAddrDataByte(cpu);
  cpu->cycleData >>= 1;
  if (cpu->flagC) {
      cpu->cycleData |= 128;
  } else {
      cpu->cycleData &= 127;
  }
  cpu->flagC = newC;
  m6510_doADC(cpu);
};
void m6510_instr74(m6510_t *cpu) {
  m6510_PopSR(cpu);
  m6510_calculateInterruptTriggerCycle(cpu);
};


void m6510_instr80(m6510_t *cpu) {
  (cpu->cpuRead)(cpu->Cycle_EffectiveAddress);
  cpu->Register_ProgramCounter = cpu->Cycle_EffectiveAddress + 1 & 65535;
}
void m6510_instr81(m6510_t *cpu) {
  cpu->cycleData = ((cpu->registerA & cpu->registerX) &0xff);
  m6510_PutEffAddrDataByte(cpu);

}
void m6510_instr82(m6510_t *cpu) {
  m6510_doSBC(cpu);
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr83(m6510_t *cpu) {
  int32_t tmp = (cpu->registerX & cpu->registerA & 255) - (cpu->cycleData & 255);
  m6510_setFlagsNZ(cpu, cpu->registerX = (tmp & 0xff));
  cpu->flagC = tmp >= 0;
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr84(m6510_t *cpu) {
  cpu->flagC = true;
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr85(m6510_t *cpu) {
  cpu->flagD = true;
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr86(m6510_t *cpu) {
  cpu->flagI = true;
  m6510_interruptsAndNextOpcode(cpu);
  if (!cpu->rstFlag && !cpu->nmiFlag && cpu->interruptCycle != M6510_MAX) {
      cpu->interruptCycle = M6510_MAX;
  }
}
void m6510_instr87(m6510_t *cpu) {
  cpu->cycleData = ((cpu->registerX & cpu->registerA & (cpu->Cycle_EffectiveAddress >> 8) + 1) & 0xff );
  if (cpu->Cycle_HighByteWrongEffectiveAddress != cpu->Cycle_EffectiveAddress) {
      cpu->Cycle_EffectiveAddress = (cpu->cycleData & 255) << 8 | (cpu->Cycle_EffectiveAddress & 255);
  }
  m6510_PutEffAddrDataByte(cpu);
}
void m6510_instr88(m6510_t *cpu) {
  cpu->registerSP = ((cpu->registerA & cpu->registerX) & 0xff);
  cpu->cycleData = (((cpu->Cycle_EffectiveAddress >> 8) + 1 & cpu->registerSP & 255) &0xff);
  if (cpu->Cycle_HighByteWrongEffectiveAddress != cpu->Cycle_EffectiveAddress) {
      cpu->Cycle_EffectiveAddress = (cpu->cycleData & 255) << 8 | (cpu->Cycle_EffectiveAddress & 255);
  }
  m6510_PutEffAddrDataByte(cpu);
}
void m6510_instr89(m6510_t *cpu) {
  cpu->cycleData = ((cpu->registerX & (cpu->Cycle_EffectiveAddress >> 8) + 1) | 0);
  if (cpu->Cycle_HighByteWrongEffectiveAddress != cpu->Cycle_EffectiveAddress) {
      cpu->Cycle_EffectiveAddress = (cpu->cycleData & 255) << 8 | (cpu->Cycle_EffectiveAddress & 255);
  }
  m6510_PutEffAddrDataByte(cpu);
}
void m6510_instr90(m6510_t *cpu) {
  cpu->cycleData = ((cpu->registerY & (cpu->Cycle_EffectiveAddress >> 8) + 1) | 0);
  if (cpu->Cycle_HighByteWrongEffectiveAddress != cpu->Cycle_EffectiveAddress) {
      cpu->Cycle_EffectiveAddress = (cpu->cycleData & 255) << 8 | (cpu->Cycle_EffectiveAddress & 255);
  }
  m6510_PutEffAddrDataByte(cpu);
}
void m6510_instr91(m6510_t *cpu) {
  m6510_PutEffAddrDataByte(cpu);
  //cpu->flagC = cpu->cycleData < 0;
  cpu->flagC = (cpu->cycleData & 0x80) > 0;
  cpu->cycleData <<= 1;
  m6510_setFlagsNZ(cpu, cpu->registerA |= cpu->cycleData);
}
void m6510_instr92(m6510_t *cpu) {
  m6510_PutEffAddrDataByte(cpu);
  cpu->flagC = (cpu->cycleData & 1) != 0;
  cpu->cycleData >>= 1;
  cpu->cycleData &= 127;
  m6510_setFlagsNZ(cpu, cpu->registerA ^= cpu->cycleData);
}
void m6510_instr93(m6510_t *cpu) {
  cpu->cycleData = cpu->registerA;
  m6510_PutEffAddrDataByte(cpu);
}
void m6510_instr94(m6510_t *cpu) {
  cpu->cycleData = cpu->registerX;
  m6510_PutEffAddrDataByte(cpu);
}
void m6510_instr95(m6510_t *cpu) {
  cpu->cycleData = cpu->registerY;
  m6510_PutEffAddrDataByte(cpu);
}
void m6510_instr96(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, cpu->registerX = cpu->registerA);
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr97(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, cpu->registerY = cpu->registerA);
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr98(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, cpu->registerX = cpu->registerSP);
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr99(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, cpu->registerA = cpu->registerX);
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr100(m6510_t *cpu) {
  cpu->registerSP = cpu->registerX;
  m6510_interruptsAndNextOpcode(cpu);
}
void m6510_instr101(m6510_t *cpu) {
  m6510_setFlagsNZ(cpu, cpu->registerA = cpu->registerY);
  m6510_interruptsAndNextOpcode(cpu);
}

void m6510_instr102(m6510_t *cpu) {
  cpu->cycleCount--; 
}


void m6510_instrDoBranch(m6510_t *cpu) {
  // hack, extra cycle if branch is taken
  //cpu->branchCycleCount = 1;

  // issue the spurious read for next insn here. 
  (cpu->cpuRead)(cpu->Register_ProgramCounter);

  // offset is signed byte
  int8_t offset = cpu->cycleData;

  // signed byte
  if( (offset & 0x80) > 0) {
    offset -= 0x100;
  }

  cpu->Cycle_HighByteWrongEffectiveAddress = (cpu->Register_ProgramCounter & 0xff00) 
                                            | (cpu->Register_ProgramCounter + offset & 0xff);
  cpu->Cycle_EffectiveAddress = cpu->Register_ProgramCounter + offset & 0xffff;

  if (cpu->Cycle_EffectiveAddress == cpu->Cycle_HighByteWrongEffectiveAddress) {
    cpu->cycleCount += 1;
    cpu->branchCycleCount = -1;
    // 
    // Hack: delay the interrupt past this instruction.
    //                                    
    if (cpu->interruptCycle >> 3 == cpu->cycleCount >> 3) {
        cpu->interruptCycle += 2;
    }
  }

  cpu->Register_ProgramCounter = cpu->Cycle_EffectiveAddress;

}

void m6510_instrBCSr(m6510_t *cpu) {
  if(cpu->flagC) {
    m6510_instrDoBranch(cpu);
  } else {
    m6510_interruptsAndNextOpcode(cpu);
  }
}

void m6510_instrBCCr(m6510_t *cpu) {
  if(!cpu->flagC) {
    m6510_instrDoBranch(cpu);
  } else {
    m6510_interruptsAndNextOpcode(cpu);
  }
}

void m6510_instrBEQr(m6510_t *cpu) {
  if(cpu->flagZ) {
    m6510_instrDoBranch(cpu);
  } else {
    m6510_interruptsAndNextOpcode(cpu);
  }
}

void m6510_instrBNEr(m6510_t *cpu) {
  if(!cpu->flagZ) {
    m6510_instrDoBranch(cpu);
  } else {
    m6510_interruptsAndNextOpcode(cpu);
  }
}

void m6510_instrBMIr(m6510_t *cpu) {
  if(cpu->flagN) {
    m6510_instrDoBranch(cpu);
  } else {
    m6510_interruptsAndNextOpcode(cpu);
  }
}

void m6510_instrBPLr(m6510_t *cpu) {
  if(!cpu->flagN) {
    m6510_instrDoBranch(cpu);
  } else {
    m6510_interruptsAndNextOpcode(cpu);
  }
}

void m6510_instrBVCr(m6510_t *cpu) {

  if(!cpu->flagV) {
    m6510_instrDoBranch(cpu);
  } else {
    m6510_interruptsAndNextOpcode(cpu);
  }
}

void m6510_instrBVSr(m6510_t *cpu) {
  
  if(cpu->flagV) {
    m6510_instrDoBranch(cpu);
  } else {
    m6510_interruptsAndNextOpcode(cpu);
  }
}


void m6510_buildInstructionTable() {
  uint32_t buildCycle = 0;
  uint32_t i;
  uint32_t access = 0;
  bool_t legalMode = true;
  bool_t legalInstr = true;

  for(i = 0; i < 256; i++) {
    buildCycle = i << 3;
    m6510_processorCycleNoSteal[buildCycle] = false;
    
    access = M6510_AccessMode_WRITE;
    legalMode = true;
    legalInstr = true;
    switch ((i)) {
      case IOpCode_ASLn:
      case IOpCode_CLCn:
      case IOpCode_CLDn:
      case IOpCode_CLIn:
      case IOpCode_CLVn:
      case IOpCode_DEXn:
      case IOpCode_DEYn:
      case IOpCode_INXn:
      case IOpCode_INYn:
      case IOpCode_LSRn:
      case IOpCode_NOPn:
      case IOpCode_NOPn_1:
      case IOpCode_NOPn_2:
      case IOpCode_NOPn_3:
      case IOpCode_NOPn_4:
      case IOpCode_NOPn_5:
      case IOpCode_NOPn_6:
      case IOpCode_PHAn:
      case IOpCode_PHPn:
      case IOpCode_PLAn:
      case IOpCode_PLPn:
      case IOpCode_ROLn:
      case IOpCode_RORn:
      case IOpCode_SECn:
      case IOpCode_SEDn:
      case IOpCode_SEIn:
      case IOpCode_TAXn:
      case IOpCode_TAYn:
      case IOpCode_TSXn:
      case IOpCode_TXAn:
      case IOpCode_TXSn:
      case IOpCode_TYAn:
        m6510_instructionTable[buildCycle++] = &m6510_readProgramCounter;
        break;
  
      case IOpCode_ADCb:
      case IOpCode_ANDb:
      case IOpCode_ANCb:
      case IOpCode_ANCb_1:
      case IOpCode_ANEb:
      case IOpCode_ASRb:
      case IOpCode_ARRb:
      case IOpCode_BCCr:
      case IOpCode_BCSr:
      case IOpCode_BEQr:
      case IOpCode_BMIr:
      case IOpCode_BNEr:
      case IOpCode_BPLr:
      case IOpCode_BRKn:
      case IOpCode_BVCr:
      case IOpCode_BVSr:
      case IOpCode_CMPb:
      case IOpCode_CPXb:
      case IOpCode_CPYb:
      case IOpCode_EORb:
      case IOpCode_LDAb:
      case IOpCode_LDXb:
      case IOpCode_LDYb:
      case IOpCode_LXAb:
      case IOpCode_NOPb:
      case IOpCode_NOPb_1:
      case IOpCode_NOPb_2:
      case IOpCode_NOPb_3:
      case IOpCode_NOPb_4:
      case IOpCode_ORAb:
      case IOpCode_RTIn:
      case IOpCode_RTSn:
      case IOpCode_SBCb:
      case IOpCode_SBCb_1:
      case IOpCode_SBXb:
        m6510_instructionTable[buildCycle++] = &m6510_instr2;
        break;
      case IOpCode_ADCz:
      case IOpCode_ANDz:
      case IOpCode_BITz:
      case IOpCode_CMPz:
      case IOpCode_CPXz:
      case IOpCode_CPYz:
      case IOpCode_EORz:
      case IOpCode_LAXz:
      case IOpCode_LDAz:
      case IOpCode_LDXz:
      case IOpCode_LDYz:
      case IOpCode_ORAz:
      case IOpCode_NOPz:
      case IOpCode_NOPz_1:
      case IOpCode_NOPz_2:
      case IOpCode_SBCz:
      case IOpCode_ASLz:
      case IOpCode_DCPz:
      case IOpCode_DECz:
      case IOpCode_INCz:
      case IOpCode_ISBz:
      case IOpCode_LSRz:
      case IOpCode_ROLz:
      case IOpCode_RORz:
      case IOpCode_SREz:
      case IOpCode_SLOz:  // ASO
      case IOpCode_RLAz:
      case IOpCode_RRAz:
        access = M6510_AccessMode_READ;
      case IOpCode_SAXz:
      case IOpCode_STAz:
      case IOpCode_STXz:
      case IOpCode_STYz:

        m6510_instructionTable[buildCycle++] = &m6510_FetchLowAddr;
        break;
      case IOpCode_ADCzx:
      case IOpCode_ANDzx:
      case IOpCode_CMPzx:
      case IOpCode_EORzx:
      case IOpCode_LDAzx:
      case IOpCode_LDYzx:
      case IOpCode_NOPzx:
      case IOpCode_NOPzx_1:
      case IOpCode_NOPzx_2:
      case IOpCode_NOPzx_3:
      case IOpCode_NOPzx_4:
      case IOpCode_NOPzx_5:
      case IOpCode_ORAzx:
      case IOpCode_SBCzx:
      case IOpCode_ASLzx:
      case IOpCode_DCPzx:
      case IOpCode_DECzx:
      case IOpCode_INCzx:
      case IOpCode_ISBzx:
      case IOpCode_LSRzx:
      case IOpCode_RLAzx:
      case IOpCode_ROLzx:
      case IOpCode_RORzx:
      case IOpCode_RRAzx:
      case IOpCode_SLOzx:
      case IOpCode_SREzx:
        access = M6510_AccessMode_READ;
      case IOpCode_STAzx:
      case IOpCode_STYzx:
        m6510_instructionTable[buildCycle++] = &m6510_FetchLowAddrX;
        m6510_instructionTable[buildCycle++] = &m6510_wastedStealable;
        break;
      case IOpCode_LDXzy:
      case IOpCode_LAXzy:
        access = M6510_AccessMode_READ;
      case IOpCode_STXzy:
      case IOpCode_SAXzy:

        m6510_instructionTable[buildCycle++] = &m6510_FetchLowAddrY;
        m6510_instructionTable[buildCycle++] = &m6510_wastedStealable;
        break;
      case IOpCode_ADCa:
      case IOpCode_ANDa:
      case IOpCode_BITa:
      case IOpCode_CMPa:
      case IOpCode_CPXa:
      case IOpCode_CPYa:
      case IOpCode_EORa:
      case IOpCode_LAXa:
      case IOpCode_LDAa:
      case IOpCode_LDXa:
      case IOpCode_LDYa:
      case IOpCode_NOPa:
      case IOpCode_ORAa:
      case IOpCode_SBCa:
      case IOpCode_ASLa:
      case IOpCode_DCPa:
      case IOpCode_DECa:
      case IOpCode_INCa:
      case IOpCode_ISBa:
      case IOpCode_LSRa:
      case IOpCode_ROLa:
      case IOpCode_RORa:
      case IOpCode_SLOa:
      case IOpCode_SREa:
      case IOpCode_RLAa:
      case IOpCode_RRAa:
        access = M6510_AccessMode_READ;
      case IOpCode_JMPw:
      case IOpCode_SAXa:
      case IOpCode_STAa:
      case IOpCode_STXa:
      case IOpCode_STYa:
        m6510_instructionTable[buildCycle++] = &m6510_FetchLowAddr;
        m6510_instructionTable[buildCycle++] = &m6510_FetchHighAddr;
        break;
      case IOpCode_JSRw:
        m6510_instructionTable[buildCycle++] = &m6510_FetchLowAddr;
        break;
      case IOpCode_ADCax:
      case IOpCode_ANDax:
      case IOpCode_CMPax:
      case IOpCode_EORax:
      case IOpCode_LDAax:
      case IOpCode_LDYax:
      case IOpCode_NOPax:
      case IOpCode_NOPax_1:
      case IOpCode_NOPax_2:
      case IOpCode_NOPax_3:
      case IOpCode_NOPax_4:
      case IOpCode_NOPax_5:
      case IOpCode_ORAax:
      case IOpCode_SBCax:
        access = M6510_AccessMode_READ;
        m6510_instructionTable[buildCycle++] = &m6510_FetchLowAddr;
        m6510_instructionTable[buildCycle++] = &m6510_instr7;
        m6510_instructionTable[buildCycle++] = &throwAwayReadStealable;
        break;
      case IOpCode_ASLax:
      case IOpCode_DCPax:
      case IOpCode_DECax:
      case IOpCode_INCax:
      case IOpCode_ISBax:
      case IOpCode_LSRax:
      case IOpCode_RLAax:
      case IOpCode_ROLax:
      case IOpCode_RORax:
      case IOpCode_RRAax:
      case IOpCode_SLOax:
      case IOpCode_SREax:
        access = M6510_AccessMode_READ;
      case IOpCode_SHYax:
      case IOpCode_STAax:
        m6510_instructionTable[buildCycle++] = &m6510_FetchLowAddr;
        m6510_instructionTable[buildCycle++] = &m6510_FetchHighAddrX;
        m6510_instructionTable[buildCycle++] = &throwAwayReadStealable;
        break;
      case IOpCode_ADCay:
        access = M6510_AccessMode_READ;

        m6510_instructionTable[buildCycle++] = &m6510_FetchLowAddr;
        m6510_instructionTable[buildCycle++] = &m6510_instr9;
        m6510_instructionTable[buildCycle++] = &throwAwayReadStealable;
        break;
      case IOpCode_ANDay:
      case IOpCode_CMPay:
      case IOpCode_EORay:
      case IOpCode_LASay:
      case IOpCode_LAXay:
      case IOpCode_LDAay:
      case IOpCode_LDXay:
      case IOpCode_ORAay:
      case IOpCode_SBCay:
        access = M6510_AccessMode_READ;
        m6510_instructionTable[buildCycle++] = &m6510_FetchLowAddr;
        m6510_instructionTable[buildCycle++] = &m6510_instr10;
        m6510_instructionTable[buildCycle++] = &throwAwayReadStealable;
        break;
      case IOpCode_DCPay:
      case IOpCode_ISBay:
      case IOpCode_RLAay:
      case IOpCode_RRAay:
      case IOpCode_SLOay:
      case IOpCode_SREay:
        access = M6510_AccessMode_READ;
      case IOpCode_SHAay:
      case IOpCode_SHSay:
      case IOpCode_SHXay:
      case IOpCode_STAay:
        m6510_instructionTable[buildCycle++] = &m6510_FetchLowAddr;
        m6510_instructionTable[buildCycle++] = &m6510_FetchHighAddrY;
        m6510_instructionTable[buildCycle++] = throwAwayReadStealable;
        break;


      case IOpCode_JMPi:
        m6510_instructionTable[buildCycle++] = &m6510_FetchLowPointer;
        m6510_instructionTable[buildCycle++] = &m6510_FetchHighPointer;
        m6510_instructionTable[buildCycle++] = &m6510_FetchLowEffAddr;
        m6510_instructionTable[buildCycle++] = &m6510_FetchHighEffAddr;
        break;
      case IOpCode_ADCix:
      case IOpCode_ANDix:
      case IOpCode_CMPix:
      case IOpCode_EORix:
      case IOpCode_LAXix:
      case IOpCode_LDAix:
      case IOpCode_ORAix:
      case IOpCode_SBCix:
      case IOpCode_DCPix:
      case IOpCode_ISBix:
      case IOpCode_SLOix:
      case IOpCode_SREix:
      case IOpCode_RLAix:
      case IOpCode_RRAix:            
        access = M6510_AccessMode_READ;
      case IOpCode_SAXix:
      case IOpCode_STAix:
        m6510_instructionTable[buildCycle++] = &m6510_FetchLowPointer;
        m6510_instructionTable[buildCycle++] = &m6510_instr16;
        m6510_instructionTable[buildCycle++] = &m6510_FetchLowEffAddr;
        m6510_instructionTable[buildCycle++] = &m6510_FetchHighEffAddr;
        break;
      case IOpCode_ADCiy:
      case IOpCode_ANDiy:
      case IOpCode_CMPiy:
      case IOpCode_EORiy:
      case IOpCode_LAXiy:
      case IOpCode_LDAiy:
      case IOpCode_ORAiy:
      case IOpCode_SBCiy:
        access = M6510_AccessMode_READ;
        m6510_instructionTable[buildCycle++] = &m6510_FetchLowPointer; 
        m6510_instructionTable[buildCycle++] = &m6510_FetchLowEffAddr;
        m6510_instructionTable[buildCycle++] = &m6510_instr17;
        m6510_instructionTable[buildCycle++] = &throwAwayReadStealable;
        break;
      case IOpCode_DCPiy:
      case IOpCode_ISBiy:
      case IOpCode_RLAiy:
      case IOpCode_RRAiy:
      case IOpCode_SLOiy:
      case IOpCode_SREiy:
        access = M6510_AccessMode_READ;
      case IOpCode_SHAiy:
      case IOpCode_STAiy:

        m6510_instructionTable[buildCycle++] = &m6510_FetchLowPointer;
        m6510_instructionTable[buildCycle++] = &m6510_FetchLowEffAddr;
        m6510_instructionTable[buildCycle++] = &m6510_FetchHighEffAddrY;
        m6510_instructionTable[buildCycle++] = &throwAwayReadStealable;
        break;
      default:
        legalMode = false;
        break;
    }

    if (access == M6510_AccessMode_READ) {
      m6510_instructionTable[buildCycle++] = &m6510_instrReadCycleData;
    }


    switch (i) {

      case IOpCode_ADCz:
      case IOpCode_ADCzx:
      case IOpCode_ADCa:
      case IOpCode_ADCax:
      case IOpCode_ADCay:
      case IOpCode_ADCix:
      case IOpCode_ADCiy:
      case IOpCode_ADCb:
        m6510_instructionTable[buildCycle++] = &m6510_instr19;
        break;
      case IOpCode_ANCb:
      case IOpCode_ANCb_1:
        m6510_instructionTable[buildCycle++] = &m6510_instr20;
        break;
      case IOpCode_ANDz:
      case IOpCode_ANDzx:
      case IOpCode_ANDa:
      case IOpCode_ANDax:
      case IOpCode_ANDay:
      case IOpCode_ANDix:
      case IOpCode_ANDiy:
      case IOpCode_ANDb:
        m6510_instructionTable[buildCycle++] = &m6510_instr21;
        break;

      case IOpCode_ANEb:
        m6510_instructionTable[buildCycle++] = &m6510_instr22;
        break;
      case IOpCode_ARRb:
        m6510_instructionTable[buildCycle++] = &m6510_instr23;
        break;
      case IOpCode_ASLn:

        m6510_instructionTable[buildCycle++] = &m6510_instr24;
        break;
      case IOpCode_ASLz:
      case IOpCode_ASLzx:
      case IOpCode_ASLa:
      case IOpCode_ASLax:

        m6510_processorCycleNoSteal[buildCycle] = true;
        m6510_instructionTable[buildCycle++] = &m6510_instr25;
        m6510_processorCycleNoSteal[buildCycle] = true;
        m6510_instructionTable[buildCycle++] = m6510_writeToEffectiveAddress;
        break;
      case IOpCode_ASRb:
          m6510_instructionTable[buildCycle++] = &m6510_instr26;
          break;
      case IOpCode_BCCr:
          m6510_instructionTable[buildCycle++] = &m6510_instrBCCr;
          m6510_instructionTable[buildCycle++] = &throwAwayReadStealable;          
          break;
      case IOpCode_BCSr:
          m6510_instructionTable[buildCycle++] = &m6510_instrBCSr;
          m6510_instructionTable[buildCycle++] = &throwAwayReadStealable;          
          break;
      case IOpCode_BEQr:
          m6510_instructionTable[buildCycle++] = &m6510_instrBEQr;
          m6510_instructionTable[buildCycle++] = &throwAwayReadStealable;          
          break;
      case IOpCode_BMIr:
          m6510_instructionTable[buildCycle++] = &m6510_instrBMIr;
          m6510_instructionTable[buildCycle++] = &throwAwayReadStealable;          
          break;
      case IOpCode_BNEr:
          m6510_instructionTable[buildCycle++] = &m6510_instrBNEr;
          m6510_instructionTable[buildCycle++] = &throwAwayReadStealable;          
          break;
      case IOpCode_BPLr:
          m6510_instructionTable[buildCycle++] = &m6510_instrBPLr;
          m6510_instructionTable[buildCycle++] = &throwAwayReadStealable;          
          break;
      case IOpCode_BVCr:
          m6510_instructionTable[buildCycle++] = &m6510_instrBVCr;
          m6510_instructionTable[buildCycle++] = &throwAwayReadStealable;          
          break;
      case IOpCode_BVSr:
          m6510_instructionTable[buildCycle++] = &m6510_instrBVSr;
          m6510_instructionTable[buildCycle++] = &throwAwayReadStealable;          
          break;


        case IOpCode_BITz:
        case IOpCode_BITa:
          m6510_instructionTable[buildCycle++] = &m6510_instr27;
          break;
        case IOpCode_BRKn:
        
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_PushHighPC;
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr29;
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr30;
            
            m6510_instructionTable[buildCycle++] = &m6510_instr31;
            m6510_instructionTable[buildCycle++] = &m6510_instr32;
            m6510_instructionTable[buildCycle++] = &m6510_fetchNextOpcode;
            break;
        case IOpCode_CLCn:
            m6510_instructionTable[buildCycle++] = &m6510_instr34;
            break;
        case IOpCode_CLDn:
            m6510_instructionTable[buildCycle++] = &m6510_instr35;
            break;
        case IOpCode_CLIn:
            m6510_instructionTable[buildCycle++] = &m6510_instr36;
            break;
        case IOpCode_CLVn:
            m6510_instructionTable[buildCycle++] = &m6510_instr37;
            break;
        case IOpCode_CMPz:
        case IOpCode_CMPzx:
        case IOpCode_CMPa:
        case IOpCode_CMPax:
        case IOpCode_CMPay:
        case IOpCode_CMPix:
        case IOpCode_CMPiy:
        case IOpCode_CMPb:
            m6510_instructionTable[buildCycle++] = &m6510_instr38;
            break;
        case IOpCode_CPXz:
        case IOpCode_CPXa:
        case IOpCode_CPXb:
            m6510_instructionTable[buildCycle++] = &m6510_instr39;
            break;
        case IOpCode_CPYz:
        case IOpCode_CPYa:
        case IOpCode_CPYb:
            m6510_instructionTable[buildCycle++] = &m6510_instr40;
            break;
        case IOpCode_DCPz:
        case IOpCode_DCPzx:
        case IOpCode_DCPa:
        case IOpCode_DCPax:
        case IOpCode_DCPay:
        case IOpCode_DCPix:
        case IOpCode_DCPiy:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr41;
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_writeToEffectiveAddress;
            break;
        case IOpCode_DECz:
        case IOpCode_DECzx:
        case IOpCode_DECa:
        case IOpCode_DECax:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr42;
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_writeToEffectiveAddress;
            break;
        case IOpCode_DEXn:
            m6510_instructionTable[buildCycle++] = &m6510_instr43;
            break;
        case IOpCode_DEYn:
            m6510_instructionTable[buildCycle++] = &m6510_instr44;
            break;
        case IOpCode_EORz:
        case IOpCode_EORzx:
        case IOpCode_EORa:
        case IOpCode_EORax:
        case IOpCode_EORay:
        case IOpCode_EORix:
        case IOpCode_EORiy:
        case IOpCode_EORb:
            m6510_instructionTable[buildCycle++] = &m6510_instr45;
            break;
        case IOpCode_INCz:
        case IOpCode_INCzx:
        case IOpCode_INCa:
        case IOpCode_INCax:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr46;
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_writeToEffectiveAddress;
            break;
        case IOpCode_INXn:
            m6510_instructionTable[buildCycle++] = &m6510_instr47;
            break;
        case IOpCode_INYn:
            m6510_instructionTable[buildCycle++] = &m6510_instr48;
            break;
        case IOpCode_ISBz:
        case IOpCode_ISBzx:
        case IOpCode_ISBa:
        case IOpCode_ISBax:
        case IOpCode_ISBay:
        case IOpCode_ISBix:
        case IOpCode_ISBiy:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr49;
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_writeToEffectiveAddress;
            break;
        case IOpCode_JSRw:
            m6510_instructionTable[buildCycle++] = &m6510_wastedStealable;
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_PushHighPC;
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_PushLowPC;
            m6510_instructionTable[buildCycle++] = &m6510_FetchHighAddr;
        case IOpCode_JMPw:
        case IOpCode_JMPi:
            m6510_instructionTable[buildCycle++] = &m6510_instr53;
            break;
        case IOpCode_LASay:
            m6510_instructionTable[buildCycle++] = &m6510_instr54;
            break;
        case IOpCode_LAXz:
        case IOpCode_LAXzy:
        case IOpCode_LAXa:
        case IOpCode_LAXay:
        case IOpCode_LAXix:
        case IOpCode_LAXiy:
            m6510_instructionTable[buildCycle++] = &m6510_instr55;
            break;
        case IOpCode_LDAz:
        case IOpCode_LDAzx:
        case IOpCode_LDAa:
        case IOpCode_LDAax:
        case IOpCode_LDAay:
        case IOpCode_LDAix:
        case IOpCode_LDAiy:
        case IOpCode_LDAb:
            m6510_instructionTable[buildCycle++] = &m6510_instr56;
            break;
        case IOpCode_LDXz:
        case IOpCode_LDXzy:
        case IOpCode_LDXa:
        case IOpCode_LDXay:
        case IOpCode_LDXb:
            m6510_instructionTable[buildCycle++] = &m6510_instr57;
            break;
        case IOpCode_LDYz:
        case IOpCode_LDYzx:
        case IOpCode_LDYa:
        case IOpCode_LDYax:
        case IOpCode_LDYb:
            m6510_instructionTable[buildCycle++] = &m6510_instr58;
            break;
        case IOpCode_LSRn:
            m6510_instructionTable[buildCycle++] = &m6510_instr59;
            break;
        case IOpCode_LSRz:
        case IOpCode_LSRzx:
        case IOpCode_LSRa:
        case IOpCode_LSRax:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr60;
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_writeToEffectiveAddress;
            break;
        case IOpCode_NOPn:
        case IOpCode_NOPn_1:
        case IOpCode_NOPn_2:
        case IOpCode_NOPn_3:
        case IOpCode_NOPn_4:
        case IOpCode_NOPn_5:
        case IOpCode_NOPn_6:
        case IOpCode_NOPb:
        case IOpCode_NOPb_1:
        case IOpCode_NOPb_2:
        case IOpCode_NOPb_3:
        case IOpCode_NOPb_4:
        case IOpCode_NOPz:
        case IOpCode_NOPz_1:
        case IOpCode_NOPz_2:
        case IOpCode_NOPzx:
        case IOpCode_NOPzx_1:
        case IOpCode_NOPzx_2:
        case IOpCode_NOPzx_3:
        case IOpCode_NOPzx_4:
        case IOpCode_NOPzx_5:
        case IOpCode_NOPa:
        case IOpCode_NOPax:
        case IOpCode_NOPax_1:
        case IOpCode_NOPax_2:
        case IOpCode_NOPax_3:
        case IOpCode_NOPax_4:
        case IOpCode_NOPax_5:
            break;
        case IOpCode_LXAb:
            m6510_instructionTable[buildCycle++] = &m6510_instr61;
            break;
        case IOpCode_ORAz:
        case IOpCode_ORAzx:
        case IOpCode_ORAa:
        case IOpCode_ORAax:
        case IOpCode_ORAay:
        case IOpCode_ORAix:
        case IOpCode_ORAiy:
        case IOpCode_ORAb:
            m6510_instructionTable[buildCycle++] = &m6510_instr62;
            break;
        case IOpCode_PHAn:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr63;
            break;
        case IOpCode_PHPn:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_PushSR;
            break;
        case IOpCode_PLAn:
            m6510_instructionTable[buildCycle++] = &m6510_wastedStealable;
            m6510_instructionTable[buildCycle++] = &m6510_instr65;
            break;
        case IOpCode_PLPn:
            m6510_instructionTable[buildCycle++] = &m6510_wastedStealable;
            m6510_instructionTable[buildCycle++] = &m6510_instr66;
            m6510_instructionTable[buildCycle++] = &m6510_interruptsAndNextOpcode;
            break;
        case IOpCode_RLAz:
        case IOpCode_RLAzx:
        case IOpCode_RLAix:
        case IOpCode_RLAa:
        case IOpCode_RLAax:
        case IOpCode_RLAay:
        case IOpCode_RLAiy:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr68;
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_writeToEffectiveAddress;
            break;
        case IOpCode_ROLn:
            m6510_instructionTable[buildCycle++] = &m6510_instr69;
            break;
        case IOpCode_ROLz:
        case IOpCode_ROLzx:
        case IOpCode_ROLa:
        case IOpCode_ROLax:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr70;
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_writeToEffectiveAddress;
            break;
        case IOpCode_RORn:
            m6510_instructionTable[buildCycle++] =  &m6510_instr71;
            break;
        case IOpCode_RORz:
        case IOpCode_RORzx:
        case IOpCode_RORa:
        case IOpCode_RORax:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr72;
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_writeToEffectiveAddress;
            break;
        case IOpCode_RRAa:
        case IOpCode_RRAax:
        case IOpCode_RRAay:
        case IOpCode_RRAz:
        case IOpCode_RRAzx:
        case IOpCode_RRAix:
        case IOpCode_RRAiy:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr73;
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_writeToEffectiveAddress;
            break;
        case IOpCode_RTIn:
            m6510_instructionTable[buildCycle++] = &m6510_wastedStealable;
            m6510_instructionTable[buildCycle++] = &m6510_instr74;
            m6510_instructionTable[buildCycle++] = &m6510_PopLowPC;
            m6510_instructionTable[buildCycle++] = &m6510_PopHighPC;
            m6510_instructionTable[buildCycle++] = &m6510_interruptEnd;
            break;
        case IOpCode_RTSn:
            m6510_instructionTable[buildCycle++] = &m6510_wastedStealable;
            m6510_instructionTable[buildCycle++] = &m6510_PopLowPC;
            m6510_instructionTable[buildCycle++] = &m6510_PopHighPC;
            m6510_instructionTable[buildCycle++] = &m6510_instr80;
            break;
        case IOpCode_SAXz:
        case IOpCode_SAXzy:
        case IOpCode_SAXa:
        case IOpCode_SAXix:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr81;
            break;
        case IOpCode_SBCz:
        case IOpCode_SBCzx:
        case IOpCode_SBCa:
        case IOpCode_SBCax:
        case IOpCode_SBCay:
        case IOpCode_SBCix:
        case IOpCode_SBCiy:
        case IOpCode_SBCb:
        case IOpCode_SBCb_1:
            m6510_instructionTable[buildCycle++] = &m6510_instr82;
            break;
        case IOpCode_SBXb:
            m6510_instructionTable[buildCycle++] = &m6510_instr83;
            break;
        case IOpCode_SECn:
            m6510_instructionTable[buildCycle++] = &m6510_instr84;
            break;
        case IOpCode_SEDn:
            m6510_instructionTable[buildCycle++] = &m6510_instr85;
            break;
        case IOpCode_SEIn:
            m6510_instructionTable[buildCycle++] = &m6510_instr86;
            break;
        case IOpCode_SHAay:
        case IOpCode_SHAiy:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr87;
            break;
        case IOpCode_SHSay:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr88;
            break;
        case IOpCode_SHXay:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr89;
            break;
        case IOpCode_SHYax:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr90;
            break;
        case IOpCode_SLOz:
        case IOpCode_SLOzx:
        case IOpCode_SLOa:
        case IOpCode_SLOax:
        case IOpCode_SLOay:
        case IOpCode_SLOix:
        case IOpCode_SLOiy:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr91;
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_writeToEffectiveAddress;
            break;
        case IOpCode_SREz:
        case IOpCode_SREzx:
        case IOpCode_SREa:
        case IOpCode_SREax:
        case IOpCode_SREay:
        case IOpCode_SREix:
        case IOpCode_SREiy:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr92;
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_writeToEffectiveAddress;
            break;
        case IOpCode_STAz:
        case IOpCode_STAzx:
        case IOpCode_STAa:
        case IOpCode_STAax:
        case IOpCode_STAay:
        case IOpCode_STAix:
        case IOpCode_STAiy:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr93;
            break;
        case IOpCode_STXz:
        case IOpCode_STXzy:
        case IOpCode_STXa:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr94;
            break;
        case IOpCode_STYz:
        case IOpCode_STYzx:
        case IOpCode_STYa:
            m6510_processorCycleNoSteal[buildCycle] = true;
            m6510_instructionTable[buildCycle++] = &m6510_instr95;
            break;
        case IOpCode_TAXn:
            m6510_instructionTable[buildCycle++] = &m6510_instr96;
            break;
        case IOpCode_TAYn:
            m6510_instructionTable[buildCycle++] = &m6510_instr97;
            break;
        case IOpCode_TSXn:
            m6510_instructionTable[buildCycle++] = &m6510_instr98;
            break;
        case IOpCode_TXAn:
            m6510_instructionTable[buildCycle++] = &m6510_instr99;
            break;
        case IOpCode_TXSn:
            m6510_instructionTable[buildCycle++] = &m6510_instr100;
            break;
        case IOpCode_TYAn:
            m6510_instructionTable[buildCycle++] = &m6510_instr101;
            break;
        default:
            legalInstr = false;
            break;
    }

    if (!(legalMode && legalInstr)) {
      m6510_instructionTable[buildCycle++] = &m6510_instr102;
    }
    m6510_instructionTable[buildCycle++] = &m6510_interruptsAndNextOpcode;
  }
}
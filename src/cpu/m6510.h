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

#ifndef M6510_H
#define M6510_H

#include "../m64.h" 

#define INSTRTABLELENGTH (256 << 3)

#define IOpCode_BRKn 0
#define IOpCode_JSRw 32
#define IOpCode_RTIn 64
#define IOpCode_RTSn 96
#define IOpCode_NOPb 128
#define IOpCode_NOPb_1 130
#define IOpCode_NOPb_2 194
#define IOpCode_NOPb_3 226
#define IOpCode_NOPb_4 137
#define IOpCode_LDYb 160
#define IOpCode_CPYb 192
#define IOpCode_CPXb 224
#define IOpCode_ORAix 1
#define IOpCode_ANDix 33
#define IOpCode_EORix 65
#define IOpCode_ADCix 97
#define IOpCode_STAix 129
#define IOpCode_LDAix 161
#define IOpCode_CMPix 193
#define IOpCode_SBCix 225
#define IOpCode_LDXb 162
#define IOpCode_SLOix 3
#define IOpCode_RLAix 35
#define IOpCode_SREix 67
#define IOpCode_RRAix 99
#define IOpCode_SAXix 131
#define IOpCode_LAXix 163
#define IOpCode_DCPix 195
#define IOpCode_ISBix 227
#define IOpCode_NOPz 4
#define IOpCode_NOPz_1 68
#define IOpCode_NOPz_2 100
#define IOpCode_BITz 36
#define IOpCode_STYz 132
#define IOpCode_LDYz 164
#define IOpCode_CPYz 196
#define IOpCode_CPXz 228
#define IOpCode_ORAz 5
#define IOpCode_ANDz 37
#define IOpCode_EORz 69
#define IOpCode_ADCz 101
#define IOpCode_STAz 133
#define IOpCode_LDAz 165
#define IOpCode_CMPz 197
#define IOpCode_SBCz 229
#define IOpCode_ASLz 6
#define IOpCode_ROLz 38
#define IOpCode_LSRz 70
#define IOpCode_RORz 102
#define IOpCode_STXz 134
#define IOpCode_LDXz 166
#define IOpCode_DECz 198
#define IOpCode_INCz 230
#define IOpCode_SLOz 7
#define IOpCode_RLAz 39
#define IOpCode_SREz 71
#define IOpCode_RRAz 103
#define IOpCode_SAXz 135
#define IOpCode_LAXz 167
#define IOpCode_DCPz 199
#define IOpCode_ISBz 231
#define IOpCode_PHPn 8
#define IOpCode_PLPn 40
#define IOpCode_PHAn 72
#define IOpCode_PLAn 104
#define IOpCode_DEYn 136
#define IOpCode_TAYn 168
#define IOpCode_INYn 200
#define IOpCode_INXn 232
#define IOpCode_ORAb 9
#define IOpCode_ANDb 41
#define IOpCode_EORb 73
#define IOpCode_ADCb 105
#define IOpCode_LDAb 169
#define IOpCode_CMPb 201
#define IOpCode_SBCb 233
#define IOpCode_SBCb_1 235
#define IOpCode_ASLn 10
#define IOpCode_ROLn 42
#define IOpCode_LSRn 74
#define IOpCode_RORn 106
#define IOpCode_TXAn 138
#define IOpCode_TAXn 170
#define IOpCode_DEXn 202
#define IOpCode_NOPn 234
#define IOpCode_NOPn_1 26
#define IOpCode_NOPn_2 58
#define IOpCode_NOPn_3 90
#define IOpCode_NOPn_4 122
#define IOpCode_NOPn_5 218
#define IOpCode_NOPn_6 250
#define IOpCode_ANCb 11
#define IOpCode_ANCb_1 43
#define IOpCode_ASRb 75
#define IOpCode_ARRb 107
#define IOpCode_ANEb 139
#define IOpCode_XAAb 139
#define IOpCode_LXAb 171
#define IOpCode_SBXb 203
#define IOpCode_NOPa 12
#define IOpCode_BITa 44
#define IOpCode_JMPw 76
#define IOpCode_JMPi 108
#define IOpCode_STYa 140
#define IOpCode_LDYa 172
#define IOpCode_CPYa 204
#define IOpCode_CPXa 236
#define IOpCode_ORAa 13
#define IOpCode_ANDa 45
#define IOpCode_EORa 77
#define IOpCode_ADCa 109
#define IOpCode_STAa 141
#define IOpCode_LDAa 173
#define IOpCode_CMPa 205
#define IOpCode_SBCa 237
#define IOpCode_ASLa 14
#define IOpCode_ROLa 46
#define IOpCode_LSRa 78
#define IOpCode_RORa 110
#define IOpCode_STXa 142
#define IOpCode_LDXa 174
#define IOpCode_DECa 206
#define IOpCode_INCa 238
#define IOpCode_SLOa 15
#define IOpCode_RLAa 47
#define IOpCode_SREa 79
#define IOpCode_RRAa 111
#define IOpCode_SAXa 143
#define IOpCode_LAXa 175
#define IOpCode_DCPa 207
#define IOpCode_ISBa 239
#define IOpCode_BPLr 16
#define IOpCode_BMIr 48
#define IOpCode_BVCr 80
#define IOpCode_BVSr 112
#define IOpCode_BCCr 144
#define IOpCode_BCSr 176
#define IOpCode_BNEr 208
#define IOpCode_BEQr 240
#define IOpCode_ORAiy 17
#define IOpCode_ANDiy 49
#define IOpCode_EORiy 81
#define IOpCode_ADCiy 113
#define IOpCode_STAiy 145
#define IOpCode_LDAiy 177
#define IOpCode_CMPiy 209
#define IOpCode_SBCiy 241
#define IOpCode_SLOiy 19
#define IOpCode_RLAiy 51
#define IOpCode_SREiy 83
#define IOpCode_RRAiy 115
#define IOpCode_SHAiy 147
#define IOpCode_LAXiy 179
#define IOpCode_DCPiy 211
#define IOpCode_ISBiy 243
#define IOpCode_NOPzx 20
#define IOpCode_NOPzx_1 52
#define IOpCode_NOPzx_2 84
#define IOpCode_NOPzx_3 116
#define IOpCode_NOPzx_4 212
#define IOpCode_NOPzx_5 244
#define IOpCode_STYzx 148
#define IOpCode_LDYzx 180
#define IOpCode_ORAzx 21
#define IOpCode_ANDzx 53
#define IOpCode_EORzx 85
#define IOpCode_ADCzx 117
#define IOpCode_STAzx 149
#define IOpCode_LDAzx 181
#define IOpCode_CMPzx 213
#define IOpCode_SBCzx 245
#define IOpCode_ASLzx 22
#define IOpCode_ROLzx 54
#define IOpCode_LSRzx 86
#define IOpCode_RORzx 118
#define IOpCode_STXzy 150
#define IOpCode_LDXzy 182
#define IOpCode_DECzx 214
#define IOpCode_INCzx 246
#define IOpCode_SLOzx 23
#define IOpCode_RLAzx 55
#define IOpCode_SREzx 87
#define IOpCode_RRAzx 119
#define IOpCode_SAXzy 151
#define IOpCode_LAXzy 183
#define IOpCode_DCPzx 215
#define IOpCode_ISBzx 247
#define IOpCode_CLCn 24
#define IOpCode_SECn 56
#define IOpCode_CLIn 88
#define IOpCode_SEIn 120
#define IOpCode_TYAn 152
#define IOpCode_CLVn 184
#define IOpCode_CLDn 216
#define IOpCode_SEDn 248
#define IOpCode_ORAay 25
#define IOpCode_ANDay 57
#define IOpCode_EORay 89
#define IOpCode_ADCay 121
#define IOpCode_STAay 153
#define IOpCode_LDAay 185
#define IOpCode_CMPay 217
#define IOpCode_SBCay 249
#define IOpCode_TXSn 154
#define IOpCode_TSXn 186
#define IOpCode_SLOay 27
#define IOpCode_RLAay 59
#define IOpCode_SREay 91
#define IOpCode_RRAay 123
#define IOpCode_SHSay 155
#define IOpCode_TASay 155
#define IOpCode_LASay 187
#define IOpCode_DCPay 219
#define IOpCode_ISBay 251
#define IOpCode_NOPax 28
#define IOpCode_NOPax_1 60
#define IOpCode_NOPax_2 92
#define IOpCode_NOPax_3 124
#define IOpCode_NOPax_4 220
#define IOpCode_NOPax_5 252
#define IOpCode_SHYax 156
#define IOpCode_LDYax 188
#define IOpCode_ORAax 29
#define IOpCode_ANDax 61
#define IOpCode_EORax 93
#define IOpCode_ADCax 125
#define IOpCode_STAax 157
#define IOpCode_LDAax 189
#define IOpCode_CMPax 221
#define IOpCode_SBCax 253
#define IOpCode_ASLax 30
#define IOpCode_ROLax 62
#define IOpCode_LSRax 94
#define IOpCode_RORax 126
#define IOpCode_SHXay 158
#define IOpCode_LDXay 190
#define IOpCode_DECax 222
#define IOpCode_INCax 254
#define IOpCode_SLOax 31
#define IOpCode_RLAax 63
#define IOpCode_SREax 95
#define IOpCode_RRAax 127
#define IOpCode_SHAay 159
#define IOpCode_LAXay 191
#define IOpCode_DCPax 223
#define IOpCode_ISBax 255


/** Status register interrupt bit. */
#define M6510_SR_INTERRUPT  2

/** Stack page location */
#define M6510_SP_PAGE 1

/**
 * IRQ/NMI magic limit values. Need to be larger than about 0x103 << 3, but
 * can't be min/max for Integer type.
 */
#define M6510_MAX 65536

#define M6510_ANE_CONST 0xee

#define M6510_AccessMode_WRITE  0
#define M6510_AccessMode_READ  1

typedef uint8_t (*cpu_read_function)(uint16_t address);
typedef void (*cpu_write_function)(uint16_t address, uint8_t value);

struct m6510 {

  cpu_read_function cpuRead;
  cpu_write_function cpuWrite;

  clock_t *clock;

  event_t eventWithSteals;
  event_t eventWithoutSteals;
  /** RDY pin state (stop CPU on read) */
  bool_t rdy;


  /** Current instruction and subcycle within instruction */
  int32_t cycleCount;
  int32_t lastCycleCount;
  int32_t branchCycleCount;
  int32_t lastCycleCountAddress;

  /**
   * When IRQ was triggered. -MAX means "during some previous instruction",
   * MAX means "no IRQ"
   */
  int32_t interruptCycle;


  /** Data regarding current instruction */
  uint32_t Cycle_EffectiveAddress;
  uint32_t Cycle_HighByteWrongEffectiveAddress;
  uint32_t Register_ProgramCounter;
  uint32_t nextOpcodeLocation;

  bool_t gotoAddress;

  uint32_t Cycle_Pointer;

  uint8_t cycleData;
  uint8_t registerA;
  uint8_t registerX;
  uint8_t registerY;
  uint8_t registerSP;

  

  // negative
  bool_t flagN;

  // carry
  bool_t flagC;

  // decimal mode
  bool_t flagD;

  // zero
  bool_t flagZ;

  // overflow
  bool_t flagV;

  // irq disable
  bool_t flagI;

  // unused
  bool_t flagU;

  // break command
  bool_t flagB;


  /** IRQ asserted on CPU */
  bool_t irqAssertedOnPin;


  /** NMI requested? */
  uint32_t nmiFlag;

  /** RST requested? */
  uint32_t rstFlag;

};


typedef struct m6510 m6510_t;

void m6510_init(m6510_t *cpu, clock_t *clock);
void m6510_triggerRST(m6510_t *cpu);
void m6510_setMemoryHandler(m6510_t *cpu, cpu_read_function read, cpu_write_function write);


void m6510_calculateInterruptTriggerCycle(m6510_t *cpu);
void m6510_interruptsAndNextOpcode(m6510_t *cpu);
void m6510_interrupt(m6510_t *cpu);
void m6510_interruptEnd(m6510_t *cpu);
void m6510_setPC(m6510_t *cpu, uint16_t address);
void m6510_fetchNextOpcode(m6510_t *cpu);

uint8_t m6510_getStatusRegister(m6510_t *cpu);
void m6510_setStatusRegister(m6510_t *cpu, uint8_t sr);

void m6510_setFlagsNZ(m6510_t *cpu, uint8_t value);

void m6510_setRDY(m6510_t *cpu, bool_t rdy);
void m6510_triggerNMI(m6510_t *cpu);
void m6510_triggerIRQ(m6510_t *cpu);
void m6510_clearIRQ(m6510_t *cpu);


void m6510_FetchLowAddr(m6510_t *cpu);
void m6510_FetchLowAddrX(m6510_t *cpu);

uint8_t m6510_getStalledOnByte(m6510_t *cpu);

typedef void (*m6510_function)(m6510_t *cpu);

void m6510_buildInstructionTable() ;

#endif
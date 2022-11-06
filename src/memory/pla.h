#ifndef MEM_H
#define MEM_H

#include "../m64.h" 

#define MEM_MAX_BANKS 16

typedef struct m6510 m6510_t;
typedef struct pla pla_t;
typedef struct cartridge cartridge_t;

void pla_init();
void pla_reset();

void pla_updateVICMaps();
void pla_updateCPUMaps();

void pla_setCpuPort(uint8_t state);
void pla_setGameExrom(bool_t gamephi1, bool_t exromphi1, bool_t gamephi2, bool_t exromphi2);
void pla_setBA(bool_t state);

void pla_setNMI(bool_t state);
void pla_setIRQ(bool_t state);

void pla_setCpu(m6510_t *cpu);
uint8_t pla_cpuRead(uint16_t address);
void pla_cpuWrite(uint16_t address, uint8_t value);

void pla_setVicMemBase(uint16_t base);

uint8_t pla_vicReadMemoryPHI2(uint16_t addr);
uint8_t pla_vicReadMemoryPHI1(uint16_t addr);
uint8_t pla_vicReadColorMemoryPHI2(uint16_t addr);


#endif
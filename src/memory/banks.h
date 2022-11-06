#ifndef BANKS_H
#define BANKS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef uint8_t (*bank_read_function)(uint16_t address);
typedef void (*bank_write_function)(uint16_t address, uint8_t value);

#define CHAR_ROM_LENGTH 0x1000
#define KERNAL_ROM_LENGTH 0x2000
#define BASIC_ROM_LENGTH 0x2000

extern uint8_t BASICROM[BASIC_ROM_LENGTH];
extern uint8_t CHARROM[CHAR_ROM_LENGTH];
extern uint8_t KERNALROM[KERNAL_ROM_LENGTH];


uint8_t basicrom_read(uint16_t address);
void basicrom_write(uint16_t address, uint8_t value);

void charrom_set(uint8_t *data, uint32_t dataLength);
uint8_t charrom_read(uint16_t address);
void charrom_write(uint16_t address, uint8_t value);


void colorram_reset();
uint8_t colorram_read(uint16_t address);
void colorram_write(uint16_t address, uint8_t value);

void io_setBank(uint8_t bank, bank_read_function readFunction, bank_write_function writeFunction);
uint8_t io_read(uint16_t address);
void io_write(uint16_t address, uint8_t value);

void kernal_reset();
void kernal_init();
bool_t kernal_getIsM64Kernal();
void kernal_write(uint16_t address, uint8_t value);
uint8_t kernal_read(uint16_t address);

void sidbank_reset();
void sidbank_write(uint16_t address, uint8_t value);
uint8_t sidbank_read(uint16_t address);

void sidbank_setMousePortEnabled(int32_t port, uint8_t enabled);
void sidbank_setPaddleX(uint8_t value);
void sidbank_setPaddleY(uint8_t value);

uint8_t *systemram_array();

void systemram_reset();
void systemram_write(uint16_t address, uint8_t value);
uint8_t systemram_read(uint16_t address);

void disconnectedbus_write(uint16_t address, uint8_t value);
uint8_t disconnectedbus_read(uint16_t address);

void colorramdisconnectedbus_write(uint16_t address, uint8_t value);
uint8_t colorramdisconnectedbus_read(uint16_t address);

void zeroram_reset();
uint8_t zeroram_read(uint16_t address);
void zeroram_write(uint16_t address, uint8_t value);


#endif
/*
 * $ZEL: plx_eeprom.h,v 1.2 2008/11/25 15:28:41 wuestner Exp $
 */

#ifndef _plx_eeprom_h_
#define _plx_eeprom_h_

#include <stdio.h>

#define USE_SERIAL

#ifdef USE_SERIAL
#define SER_WORDS 2
#else
#define SER_WORDS 0
#endif

#define EEPROM_LEN_9054 0x2c
#define EEPROM_LEN_8311 0x33
#define MAX_EEPROM_LEN (EEPROM_LEN_8311+SER_WORDS)

extern char* eeprom_names[EEPROM_LEN_8311];
extern char* eeprom_names_ser[SER_WORDS];

int eeprom_size(int p);
int eeprom_read(int p, u_int16_t* data, int start, int len);
void eeprom_dump(u_int16_t* data, int start, int len, int eepromsize, int v);
int read_eeprom_data_from_file(FILE* f, u_int16_t* data, u_int16_t* addr, int* len);
int eeprom_write(int p, u_int16_t* data, int start, int len);

#endif

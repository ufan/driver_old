/*
 * $ZEL: eeprom_tools.c,v 1.4 2008/11/25 15:28:41 wuestner Exp $
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define __CONCAT(x,y)	x ## y
#define __CC(x, y) __CONCAT(x,y)
#define __CONCAT3(x,y,z) __CC(x, __CC(y,z))
#define __STRING(x)	#x
#define __SS(s) __STRING(s)

#include __SS(__CC(DRIVER, _var.h))
#include "plx_eeprom.h"

void
eeprom_dump(u_int16_t* data, int start, int len, int eepromsize, int v)
{
    int i, idx;

    for (i=0, idx=start; i<len; i++, idx++) {
        printf("0x%02x: 0x%04x", (i+start)*2, data[i]);
        if (v) {
            if (i<eepromsize)
                printf("  # %s", eeprom_names[idx]);
            else
                printf("  # %s", eeprom_names_ser[idx-eepromsize]);
        }
        printf("\n");
    }
}

int
eeprom_size(int p)
{
    int size, res;

    res=ioctl(p, __CC(IONAME, _EEPROM_SIZE), &size);
    if (res<0) {
        if (errno==ENOTTY) {
            size=EEPROM_LEN_9054; /* old driver */
        } else {
            fprintf(stderr, "ioctl(EEPROM_SIZE): %s\n", strerror(errno));
            return -1;
        }
    }
    return size;
}

int
eeprom_read(int p, u_int16_t* data, int start, int len)
{
    int res;
    struct __CC(DRIVER, _eeprom_req) eeprom_req;

    eeprom_req.num=len;
    eeprom_req.data=data;
    eeprom_req.addr=start;
    res=ioctl(p, __CC(IONAME, _READ_EEPROM), &eeprom_req);
    if (res<0) {
        fprintf(stderr, "ioctl(READ_EEPROM): %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int
eeprom_write(int p, u_int16_t* data, int start, int len)
{
    int res;
    struct __CC(DRIVER, _eeprom_req) eeprom_req;

    eeprom_req.num=len;
    eeprom_req.data=data;
    eeprom_req.addr=start;
    res=ioctl(p, __CC(IONAME, _WRITE_EEPROM), &eeprom_req);
    if (res<0) {
        fprintf(stderr, "ioctl(WRITE_EEPROM): %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int
read_eeprom_data_from_file(FILE* f, u_int16_t* data, u_int16_t* addr, int* len)
{
    char s[1024];
    int a, val, i=0;

    while (fgets(s, 1024, f)) {
        if (sscanf(s, "%i:%i", &a, &val)!=2) {
            fprintf(stderr, "cannot convert \"%s\".\n", s);
            return -1;
        }
        if (i>=*len) {
            fprintf(stderr, "too many data in file.\n");
            return -1;
        }
        data[i]=val;
        addr[i]=a;
        i++;
    }
    *len=i;
    return 0;
}

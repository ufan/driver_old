/*
 * $ZEL: write_eeprom.c,v 1.2 2008/11/25 15:28:41 wuestner Exp $
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

#include "plx_eeprom.h"

int verbose;
const char* pathname;

static void
printhelp(const char* progname)
{
    fprintf(stderr, "usage: %s [-h] [-v] path_to_device\n", progname);
}

static int
getoptions(int argc, char* argv[])
{
    extern char *optarg;
    extern int optind;
    int errflag, c;
    const char* args="hv";

    optarg=0; errflag=0;
    verbose=0;

    while (!errflag && ((c=getopt(argc, argv, args))!=-1)) {
        switch (c) {
        case 'h': errflag++; break;
        case 'v': verbose++; break;
        default: errflag++;
        }
    }

    if (errflag || (argc-optind)!=1) {
        printhelp(argv[0]);
        return -1;
    }

    pathname=argv[optind];

    return 0;
}

int main(int argc, char* argv[])
{
    u_int16_t data_prom[MAX_EEPROM_LEN];
    u_int16_t data_file[MAX_EEPROM_LEN];
    u_int16_t addr[MAX_EEPROM_LEN];
    int p, eepromsize, num, i, n;

    fprintf(stderr, "\n==== SIS1100 EEPROM Writer; V1.0 ====\n\n");

    if (getoptions(argc, argv)) return 1;

    p=open(pathname, O_RDWR, 0);
    if (p<0) {
        fprintf(stderr, "open \"%s\": %s\n", pathname, strerror(errno));
        return 2;
    }

    eepromsize=eeprom_size(p);
    if (eepromsize<0)
        return 2;

    eepromsize+=SER_WORDS;
    num=eepromsize;
    if (read_eeprom_data_from_file(stdin, data_file, addr, &num)<0) return 1;

    if (eeprom_read(p, data_prom, 0, eepromsize)<0) return 3;

    for (i=0; i<num; i++) {
        u_int16_t a=addr[i]>>1;
        if (a>=eepromsize) {
            fprintf(stderr, "invalid address 0x%04x.\n", addr[i]);
            return 1;
        }
        data_prom[a]=data_file[i];
    }

    if (eeprom_write(p, data_prom, 0, eepromsize)<0) return 3;
    sleep(2);
    if (eeprom_read(p, data_prom, 0, eepromsize)<0) return 3;

    close(p);

    for (i=0, n=0; i<num; i++) {
        u_int16_t a=addr[i]>>1;
        if (data_prom[a]!=data_file[i]) {
            if (!n) {
                printf("Differences after write:\n");
                printf("addr eprom file\n");
            }
            printf("0x%02x: %04x %04x", a*2, data_prom[a], data_file[i]);
            if (verbose) printf("  # %s", eeprom_names[a]);
            printf("\n");
            n++;
        }
    }
    printf("eeprom %ssuccessfully written.\n", n?"NOT ":"");
    return n?10:0;
}

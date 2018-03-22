/*
 * $ZEL: read_eeprom.c,v 1.2 2008/11/25 15:28:41 wuestner Exp $
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

int verbose, start, len;
const char* pathname;

static void
printhelp(const char* progname)
{
    fprintf(stderr, "usage: %s [-h] [-v] [-a start] [-l len] path_to_device\n", progname);
}

static int
getoptions(int argc, char* argv[])
{
    extern char *optarg;
    extern int optind;
    int errflag, c;
    const char* args="a:l:hv";

    optarg=0; errflag=0;
    verbose=0;
    start=0;
    len=-1;

    while (!errflag && ((c=getopt(argc, argv, args))!=-1)) {
        switch (c) {
        case 'h': errflag++; break;
        case 'v': verbose++; break;
        case 'a': start=strtol(optarg, 0, 0); break;
        case 'l': len=strtol(optarg, 0, 0); break;
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
    u_int16_t data[MAX_EEPROM_LEN];
    int p, eepromsize, xeepromsize;

    fprintf(stderr, "\n==== SIS1100 EEPROM Reader; V1.0 ====\n\n");

    if (getoptions(argc, argv)) return 1;

    p=open(pathname, O_RDWR, 0);
    if (p<0) {
        fprintf(stderr, "open \"%s\": %s\n", pathname, strerror(errno));
        return 2;
    }

    eepromsize=eeprom_size(p);
    if (eepromsize<0)
        return 2;

    xeepromsize=eepromsize+SER_WORDS;
    start>>=1;
    if (start>=xeepromsize) {
        fprintf(stderr, "start must be less than %d\n", xeepromsize<<1);
        return 1;
    }

    if ((len<0) || (start+len>xeepromsize)) {
        len=xeepromsize-start;
    }

    if (eeprom_read(p, data, start, len)<0) return 3;
    close(p);
    eeprom_dump(data, start, len, eepromsize, verbose);

    return 0;
}

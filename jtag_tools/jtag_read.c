/*
 * $ZEL: jtag_read.c,v 1.1 2003/08/30 21:23:18 wuestner Exp $
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "jtag_tools.h"

#ifndef MAP_VARIABLE
#define MAP_VARIABLE 0x0000
#endif

static const char* device_name;
static const char* file_name;
static int chip_index;

static void
printhelp(const char* progname)
{
    fprintf(stderr, "usage: %s [-h] [-d [-d ...]] [-s] [-c chip_idx] [-f filename] "
            "device\n",
            progname);
}

static int
getoptions(int argc, char* argv[])
{
    extern char *optarg;
    extern int optind;
    int errflag, c;
    const char* args="hdc:f:s";

    optarg=0; errflag=0;
    debug=0;
    device_name=0;
    file_name=0;
    chip_index=-1;
    simulate=0;

    while (!errflag && ((c=getopt(argc, argv, args))!=-1)) {
        switch (c) {
        case 'h': errflag++; break;
        case 'd': debug++; break;
        case 's': simulate++; break;
        case 'c': {
                char* end;
                chip_index=strtol(optarg, &end, 0);
                if (*end) {
                    fprintf(stderr, "cannot convert %s\n", optarg);
                    errflag++;
                }
            }
            break;
        case 'f': file_name=optarg; break;
        default: errflag++;
        }
    }

    if (errflag || ((argc-optind)!=1) || !file_name || (chip_index<0)) {
        printhelp(argv[0]);
        return -1;
    }

    device_name=argv[optind];
    if (simulate) debug++;

    return 0;
}

int main(int argc, char* argv[])
{
    struct jtag_tab jtab={.num_devs=0, .devs=0};
    void* mp;
    int p, size;
    char null=0;

    fprintf(stderr, "\n==== SIS1100 JTAG Read; V0.1 ====\n\n");

    if (getoptions(argc, argv)<0) return 1;

    jtab.p=open(device_name, O_RDWR, 0);
    if (jtab.p<0) {
        fprintf(stderr, "open \"%s\": %s\n", device_name, strerror(errno));
        return 1;
    }

    if ((jtag_init(&jtab))<=0) {
        fprintf(stderr, "no JTAG chain found\n");
        close(jtab.p);
        return 2;
    }

    jtag_print_ids(&jtab);

    if (chip_index>=jtab.num_devs) {
        fprintf(stderr, "illegal device index (%d)\n", chip_index);
        close(jtab.p);
        return 1;
    }

    switch (jtab.devs[chip_index].id&0x0fffffff) {
        case ID_XC18V01: size=0x20000; break;
        case ID_XC18V02: size=0x40000; break;
        case ID_XC18V04: size=0x80000; break;
        default:
            fprintf(stderr, "wrong device: %s\n", jtab.devs[chip_index].name);
            close(jtab.p);
            return 1;
    }

    p=open(file_name, O_RDWR|O_CREAT|O_TRUNC/*|O_EXCL*/, 0644);
    if (p<0) {
        fprintf(stderr, "open \"%s\": %s\n", file_name, strerror(errno));
        close(jtab.p);
        return 1;
    }

    if (pwrite(p, &null, 1, size-1)!=1) {
        fprintf(stderr, "cannot write to \"%s\": %s\n",
                file_name, strerror(errno));
        close(p);
        close(jtab.p);
        return 1;
    }

    mp=mmap(0, size, PROT_READ|PROT_WRITE,
            MAP_VARIABLE|MAP_FILE|MAP_SHARED, p, 0);
    if (mp==MAP_FAILED) {
        fprintf(stderr, "cannot mmap \"%s\": %s\n",
                file_name, strerror(errno));
        close(p);
        close(jtab.p);
        return -1;
    }

    jtag_read_XC18V00(jtab.devs+chip_index, mp, size);

    munmap(mp, size);
    close(p);

    close(jtab.p);

    return 0;
}

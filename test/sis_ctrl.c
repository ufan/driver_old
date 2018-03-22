/* $ZEL: sis_ctrl.c,v 1.2 2010/08/02 19:25:39 wuestner Exp $ */

#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "dev/pci/sis1100_var.h"

enum operation {op_none, op_r, op_w, op_rw, op_wr};

const char *devname="/dev/sis1100_00ctrl";
unsigned long int addr;
unsigned long int val;
enum operation operation=op_none;

/****************************************************************************/
static void
usage(const char *argv0)
{
    fprintf(stderr, "usage: %s -f dev [-r] [-w] addr [value]\n", argv0);
    fprintf(stderr, "  -f: device file to be used "
                    "(default /dev/sis1100_00ctrl)\n");
    fprintf(stderr, "  -r: read (default if only addr is given)\n");
    fprintf(stderr, "  -w: write (default if addr and val are given\n");
    fprintf(stderr, "    -w -r: write and read back\n");
    fprintf(stderr, "    -r -w: read old value and write\n");
}
/****************************************************************************/
static int
getoptions(int argc, char *argv[])
{
    int c, err=0;
    int addr_given=0, val_given=0;

    optarg=0;
    while (!err && ((c=getopt(argc, argv, "f:wr")) != -1)) {
        switch (c) {
        case 'r':
            if (operation==op_w)
                operation=op_wr;
            else
                operation=op_r;
            break;
        case 'w':
            if (operation==op_r)
                operation=op_rw;
            else
                operation=op_w;
            break;
        case 'f':
            devname=optarg;
            break;
        default:
            err++;
        }
    }

    if (optind<argc) {
        addr=strtoul(argv[optind], 0, 0);
        addr_given=1;
        optind++;
    }
    if (optind<argc) {
        val=strtol(argv[optind], 0, 0);
        val_given=1;
        optind++;
    }

    if (operation==op_none)
        operation=val_given?op_w:op_r;

    if (!addr_given)
        err++;
    if (operation!=op_r && !val_given)
        err++;

    if (err)
        usage(argv[0]);

#if 1
    fprintf(stderr, "devname   = %s\n", devname);
    fprintf(stderr, "operation = %d\n", operation);
    if (addr_given)
        fprintf(stderr, "addr      = 0x%lx\n", addr);
    if (val_given)
        fprintf(stderr, "val       = 0x%lx\n", val);
#endif

    return err;
}
/****************************************************************************/
static void
to_bits(u_int32_t val, char *bits)
{
    int i;

    for (i=38; i>-1; i--) {
        if (i%5==4) {
            bits[i]=' ';
        } else {
            bits[i]='0'+(val&1);
            val>>=1;
        }
    }
    bits[39]=0;
}
/****************************************************************************/
static void
print_val(u_int32_t val)
{
    char bits[40];
    to_bits(val, bits);
    printf("%08x %9d %s\n", val, val, bits);
}
/****************************************************************************/
static int
sis_read(int p, u_int32_t addr, u_int32_t *val)
{
    struct sis1100_ctrl_reg reg;

    reg.offset=addr;
    if (ioctl(p, SIS1100_CTRL_READ, &reg)<0) {
        fprintf(stderr, "read(0x%x): %s\n", addr, strerror(errno));
        return -1;
    }
    if (reg.error) {
        fprintf(stderr, "read(0x%x): error 0x%x\n", addr, reg.error);
        return -1;
    }
    *val=reg.val;
    return 0;
}
/****************************************************************************/
static int
sis_write(int p, u_int32_t addr, u_int32_t val)
{
    struct sis1100_ctrl_reg reg;

    reg.offset=addr;
    if (ioctl(p, SIS1100_CTRL_WRITE, &reg)<0) {
        fprintf(stderr, "write(0x%x, 0x%x): %s\n", addr, val, strerror(errno));
        return -1;
    }
    if (reg.error) {
        fprintf(stderr, "write(0x%x, 0x%x): error 0x%x\n", addr, val, reg.error);
        return -1;
    }
    return 0;
}
/****************************************************************************/
int
main(int argc, char *argv[])
{
    u_int32_t rval;
    int p, res=0;

    if (getoptions(argc, argv))
        return 1;

    if ((p=open(devname, O_RDWR, 0))<0) {
        fprintf(stderr, "open %s: %s\n", devname, strerror(errno));
        return 2;
    }

    switch (operation) {
    case op_r:
        res=sis_read(p, addr, &rval);
        print_val(rval);
        break;
    case op_w:
        res=sis_write(p, addr, val);
        break;
    case op_rw:
        res=sis_read(p, addr, &rval);
        print_val(rval);
        res=sis_write(p, addr, val);
        break;
    case op_wr:
        res=sis_write(p, addr, val);
        res=sis_read(p, addr, &rval);
        print_val(rval);
        break;
    }

    return res?5:0;
}

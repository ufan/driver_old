/* $ZEL$ */

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

enum operation {op_none, op_r, op_w};

const char *devname="/dev/sis1100_00ctrl";
u_int32_t *vals;
u_int32_t addr;
u_int32_t count;
int fifo, p;
enum operation operation=op_none;

/****************************************************************************/
static void
usage(const char *argv0)
{
    fprintf(stderr, "usage: %s -f dev -n count [-c] addr [value ...]\n", argv0);
    fprintf(stderr, "  -f: device file to be used "
                    "(default /dev/sis1100_00ctrl)\n");
    fprintf(stderr, "  -n: number of words to be read or written\n"
                    "      if it is ommitted the number of 'values' is used\n");
    fprintf(stderr, "  -c: constant address (FIFO mode)\n");
    fprintf(stderr, "  addr: start address\n");
    fprintf(stderr, "  value ...: one or more values to be written\n"
                    "      if count does not match the number of given values "
                    "then superfluous values are ignored or the last value "
                    "is repeated\n");
}
/****************************************************************************/
static int
getoptions(int argc, char *argv[])
{
    int c, err=0;
    int xcount;
    vals=0;
    count=0;
    fifo=0;

    optarg=0;
    while (!err && ((c=getopt(argc, argv, "f:n:c")) != -1)) {
        switch (c) {
        case 'f':
            devname=optarg;
            break;
        case 'n':
            count=strtoul(optarg, 0, 0);
            break;
        case 'c':
            fifo=1;
            break;
        default:
            err++;
        }
    }
    if (optind<argc) {
        addr=strtoul(argv[optind], 0, 0);
        optind++;
    } else {
        fprintf(stderr, "address is required\n");
        err++;
    }

    if (err)
        goto error;

    xcount=argc-optind;

    if (xcount>0) {
        u_int32_t v;
        int i;
        operation=op_w;
        if (!count)
            count=xcount;
        vals=calloc(count, sizeof(u_int32_t));
        if (!vals) {
            fprintf(stderr, "allocate %d words: %s\n", count, strerror(errno));
            return -1;
        }
        for (i=0; i<xcount && i<count; i++)
            vals[i]=strtoul(argv[optind+i], 0, 0);
        for (v=vals[i-1]; i<count; i++)
            vals[i]=v;
    } else {
        operation=op_r;
        if (!count)
            count=1;
        vals=calloc(count, sizeof(u_int32_t));
        if (!vals) {
            fprintf(stderr, "allocate %d words: %s\n", count, strerror(errno));
            return -1;
        }
    }

error:
    if (err)
        usage(argv[0]);

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
    printf("0x%08x %9d %s\n", val, val, bits);
}
/****************************************************************************/
static int
do_read(void)
{
    struct sis1100_ctrl_rw reg;
    int i;

    reg.offset=addr;
    reg.fifo=fifo;
    reg.count=count;
    reg.data=vals;
    if (ioctl(p, SIS1100_CTRL_READ_BLOCK, &reg)<0) {
        fprintf(stderr, "read: %s\n", strerror(errno));
        return -1;
    }
    if (reg.error) {
        fprintf(stderr, "read: error 0x%x\n", reg.error);
        return -1;
    }
    for (i=0; i<count; i++)
        print_val(vals[i]);
    return 0;
}
/****************************************************************************/
static int
do_write(void)
{
    struct sis1100_ctrl_rw reg;

    reg.offset=addr;
    reg.fifo=fifo;
    reg.count=count;
    reg.data=vals;
    if (ioctl(p, SIS1100_CTRL_WRITE_BLOCK, &reg)<0) {
        fprintf(stderr, "write: %s\n", strerror(errno));
        return -1;
    }
    if (reg.error) {
        fprintf(stderr, "write: error 0x%x\n", reg.error);
        return -1;
    }
    return 0;
}
/****************************************************************************/
int
main(int argc, char *argv[])
{
    int res=0;

    if (getoptions(argc, argv))
        return 1;

    if ((p=open(devname, O_RDWR, 0))<0) {
        fprintf(stderr, "open %s: %s\n", devname, strerror(errno));
        return 2;
    }

    switch (operation) {
    case op_w:
        res=do_write();
        break;
    case op_r:
        res=do_read();
        break;
    }
    return res?5:0;
}
/****************************************************************************/
/****************************************************************************/

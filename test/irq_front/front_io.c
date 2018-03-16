/* $ZEL$ */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <sis1100_var.h>

enum bit_op {bit_set, bit_reset, bit_pulse};

/****************************************************************************/
int
main(int argc, char* argv[])
{
    int p;
    u_int32_t bitmask;
    enum bit_op bit_op=bit_set;

    if (argc!=4) {
        printf("usage: %s path bitmask set|reset|pulse\n", argv[0]);
        return 1;
    }

    bitmask=strtol(argv[2], 0, 0);
    if (argv[3][0]=='s') {
        bit_op=bit_set;
    } else if (argv[3][0]=='r') {
        bit_op=bit_reset;
    } else if (argv[3][0]=='p') {
        bit_op=bit_pulse;
    } else {
        printf("unknown code \"%s\"\n", argv[3]);
        return 1;
    }

    if ((p=open(argv[1], O_RDWR, 0))<0) {
        printf("open %s: %s\n", argv[1], strerror(errno));
        return 2;
    }

    switch (bit_op) {
    case bit_reset:
        bitmask<<=16;
        /* no break */
    case bit_set:
        if (ioctl(p, SIS1100_FRONT_IO, &bitmask)<0) {
            printf("ioctl(SIS1100_FRONT_IO): %s\n", strerror(errno));
            return 3;    
        }
        break;
    case bit_pulse:
        if (ioctl(p, SIS1100_FRONT_PULSE, &bitmask)<0) {
            printf("ioctl(SIS1100_FRONT_PULSE): %s\n", strerror(errno));
            return 3;    
        }
        break;
    }

    close(p);
    return 0;
}

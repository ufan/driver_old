/*
 * $ZEL: jtag_ids.c,v 1.1 2003/08/30 21:23:17 wuestner Exp $
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include "jtag_tools.h"

int main(int argc, char* argv[])
{
    struct jtag_tab jtab={.num_devs=0, .devs=0};
    int p;

    debug=0;

    fprintf(stderr, "\n==== SIS1100 JTAG Test; V0.1 ====\n\n");

    if (argc!=2) {
        fprintf(stderr, "usage: %s pathname\n", argv[0]);
        return 1;
    }

    p=open(argv[1], O_RDWR, 0);
    if (p<0) {
        fprintf(stderr, "open \"%s\": %s\n", argv[1], strerror(errno));
        return 1;
    }
    jtab.p=p;

    if ((jtag_init(&jtab))<=0) {
        fprintf(stderr, "no JTAG chain found\n");
        return 2;
    }

    jtag_print_ids(&jtab);

    close(p);
    return 0;
}

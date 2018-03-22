/* $ZEL: irq_link_select.c,v 1.2 2010/08/02 19:25:39 wuestner Exp $ */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/select.h>

#include <sis1100_var.h>

/****************************************************************************/
int
main(int argc, char* argv[])
{
    int p, count=10;

    if (argc!=2) {
        printf("usage: %s path_to_the_ctrl_device (i.e. /dev/sis1100_00ctrl)\n",
                argv[0]);
        return 1;
    }

    if ((p=open(argv[1], O_RDWR, 0))<0) {
        printf("open %s: %s\n", argv[1], strerror(errno));
        return 2;
    }

    while (count--) {
        struct sis1100_irq_get irqget;
        fd_set readfds;
        int res;

        FD_ZERO(&readfds);
        FD_SET(p, &readfds);

        res=select(p+1, &readfds, 0, 0, 0);
        if (res<0) {
            printf("select: %s\n", strerror(errno));
            return 4;
        }
        if (!FD_ISSET(p, &readfds)) {
            printf("select succeeded but bit in fd_set is not set\n");
            return 4;
        }

        res=read(p, &irqget, sizeof(irqget));
        if (res<0) {
            printf("read: %s\n", strerror(errno));
            return 4;
        }

        switch (irqget.remote_status) {
        case -1:
            printf("Link down\n");
            break;
        case 1:
            printf("Link up\n");
            break;
        default:
            printf("got irq, but link state is unchanged\n");
        }
    }

    close(p);
    return 0;    
}
/****************************************************************************/
/****************************************************************************/

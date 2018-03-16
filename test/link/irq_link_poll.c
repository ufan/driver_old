/* $ZEL: irq_link_poll.c,v 1.1 2009/04/13 00:13:30 wuestner Exp $ */

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

/****************************************************************************/
int
main(int argc, char* argv[])
{
    int p, count=10;
    /*struct sis1100_irq_ctl irqctl;*/
    struct sis1100_irq_get irqget;
    /*struct sis1100_irq_ack irqack;*/

    if (argc!=2) {
        printf("usage: %s path\n", argv[0]);
        return 1;
    }

    if ((p=open(argv[1], O_RDWR, 0))<0) {
        printf("open %s: %s\n", argv[1], strerror(errno));
        return 2;
    }

#if 0
/* IRQ_CTL is not necessary if we are interested in link up/down IRQs only
   and no signal has to be delivered */
    irqctl.irq_mask=0;
    irqctl.signal=-1; /* activated, but no signal */
    if (ioctl(p, SIS1100_IRQ_CTL, &irqctl)<0) {
        printf("ioctl(SIS1100_IRQ_CTL): %s\n", strerror(errno));
        return 3;    
    }
#endif

    while (count--) {
        /* This example is idiotic; please don't use it */
        do {
            irqget.irq_mask=0;
            if (ioctl(p, SIS1100_IRQ_GET, &irqget)<0) {
                printf("ioctl(SIS1100_IRQ_GET): %s\n", strerror(errno));
                return 1;
            }
        } while (!irqget.remote_status);

        switch (irqget.remote_status) {
            case -1:
                printf("Link down\n");
                break;
            case 1:
                printf("Link up\n");
                break;
            default:
                printf("impossible case: code broken\n");
        }
#if 0
/* IRQ_ACK is not necessary if we are interested in link up/down IRQs only */
        irqack.irq_mask=irqget.irqs;
        if (ioctl(p, SIS1100_IRQ_ACK, &irqack)<0) {
            printf("ioctl(SIS1100_IRQ_ACK): %s\n", strerror(errno));
            return 1;
        }
#endif
    }

    close(p);
    return 0;    
}
/****************************************************************************/
/****************************************************************************/

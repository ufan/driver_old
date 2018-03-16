#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "dev/pci/sis1100_var.h"

/****************************************************************************/
int main(int argc, char* argv[])
{
    int p, count=0;
    struct sis1100_irq_ctl irqctl;
    struct sis1100_irq_get irqget;

    if (argc!=2)
        {
        fprintf(stderr, "usage: %s path\n", argv[0]);
        return 1;
        }

    if ((p=open(argv[1], O_RDWR, 0))<0) return 1;

    irqctl.irq_mask=0;
    irqctl.signal=-1;
    if (ioctl(p, SIS1100_IRQ_CTL, &irqctl)<0) {
        fprintf(stderr, "ioctl(SIS1100_IRQ_CTL): %s\n", strerror(errno));
        return 1;    
    }

    while (count++<10) {
        irqget.irq_mask=0;
        irqget.immediate_ack=0;
        if (ioctl(p, SIS1100_IRQ_WAIT, &irqget)<0) {
            fprintf(stderr, "ioctl(SIS1100_IRQ_WAIT): %s\n", strerror(errno));
            return 1;
        }
        printf("  SIS1100_IRQ_WAIT returned; irqs=%08x, remote_status=%d\n",
                irqget.irqs, irqget.remote_status);
    }

    close(p);
    return 0;    
}
/****************************************************************************/
/****************************************************************************/

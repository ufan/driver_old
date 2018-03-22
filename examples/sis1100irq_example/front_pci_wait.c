#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "dev/pci/sis1100_var.h"

/****************************************************************************/
int main(int argc, char* argv[])
{
    int p, count=0;
    u_int32_t iobits;
    struct sis1100_irq_ctl irqctl;
    struct sis1100_irq_get irqget;

    if (argc!=2) {
        printf("usage: %s path\n", argv[0]);
        return 1;
    }

    if ((p=open(argv[1], O_RDWR, 0))<0) {
        printf("open %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    /* activate LEMO outputs and user LEDs on PCI board */
    iobits=0xf<<8; /* see comment in sis1100_front_io.c */
    if (ioctl(p, SIS1100_FRONT_IO, &iobits)<0) {
        printf("ioctl(SIS1100_FRONT_IO): %s\n", strerror(errno));
        return 1;    
    }

    /* enable SIS1100 FRONT IRQS */
    irqctl.irq_mask=SIS1100_FRONT_IRQS;
    irqctl.signal=-1;
    if (ioctl(p, SIS1100_IRQ_CTL, &irqctl)<0) {
        printf("ioctl(SIS1100_IRQ_CTL): %s\n", strerror(errno));
        return 1;    
    }

    while (count++<10) {
        irqget.irq_mask=SIS1100_FRONT_IRQS;
        if (ioctl(p, SIS1100_IRQ_WAIT, &irqget)<0) {
            printf("ioctl(SIS1100_IRQ_WAIT): %s\n", strerror(errno));
            return 1;
        }
        /* link status messages are always delivered */
        if (irqget.remote_status!=0) {
            printf("o%sline\n", irqget.remote_status>0?"n":"ff");
        }

        /* read actual status */
        iobits=0;
        if (ioctl(p, SIS1100_FRONT_IO, &iobits)<0) {
            printf("ioctl(SIS1100_FRONT_IO): %s\n", strerror(errno));
            return 1;    
        }
        printf("irqs=%08x, status=%08x\n", irqget.irqs, iobits);

        /*irqget.irqs&=SIS1100_FRONT_IRQS;*/
        if (ioctl(p, SIS1100_IRQ_ACK, &irqget.irqs)<0) {
            printf("ioctl(SIS1100_IRQ_ACK): %s\n", strerror(errno));
            return 1;
        }
    }

    /* disable SIS1100 FRONT IRQS */
    irqctl.irq_mask=SIS1100_FRONT_IRQS;
    irqctl.signal=0;
    if (ioctl(p, SIS1100_IRQ_CTL, &irqctl)<0) {
        printf("ioctl(SIS1100_IRQ_CTL): %s\n", strerror(errno));
        return 1;    
    }

    /* deactivate LEMO outputs */
    iobits=0xf<<24;
    if (ioctl(p, SIS1100_FRONT_IO, &iobits)<0) {
        printf("ioctl(SIS1100_FRONT_IO): %s\n", strerror(errno));
        return 1;    
    }

    close(p);
    return 0;    
}
/****************************************************************************/
/****************************************************************************/

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

int main(int argc, char* argv[])
{
    int p, count=0;
    u_int32_t iobits;
    struct sis1100_irq_ctl irqctl;
    struct sis1100_irq_get irqget;
    enum sis1100_subdev subdev;

    if (argc!=2) {
        printf("usage: %s ctrl_path\n", argv[0]);
        return 1;
    }

    if ((p=open(argv[1], O_RDWR, 0))<0) {
        printf("open %s: %s", argv[1], strerror(errno));
        return 1;
    }
    if (ioctl(p, SIS1100_DEVTYPE, &subdev)<0) {
        printf("ioctl(SIS1100_DEVTYPE): %s\n", strerror(errno));
        return 1;    
    }
    if (subdev!=sis1100_subdev_ctrl) {
        printf("The sis1100 control subdevice has to be used!\n");
        return 1;
    }

    /* activate all outputs */
    iobits=0xff; /* see sis3100rem_front_io.c */
    if (ioctl(p, SIS1100_FRONT_IO, &iobits)<0) {
        printf("ioctl(SIS1100_FRONT_IO): %s\n", strerror(errno));
        return 1;    
    }

    /* enable SIS3100 FRONT IRQS */
    irqctl.irq_mask=SIS3100_FRONT_IRQS;
    irqctl.signal=-1;
    if (ioctl(p, SIS1100_IRQ_CTL, &irqctl)<0) {
        printf("ioctl(SIS1100_IRQ_CTL): %s\n", strerror(errno));
        return 1;    
    }

    while (count++<10) {
        fd_set readfds;
        int res;

        FD_ZERO(&readfds);
        FD_SET(p, &readfds);
        res=select(p+1, &readfds, 0, 0, 0);
        if (res<0) {
            printf("select: %s\n", strerror(errno));
            return 1;    
        }

        res=read(p, &irqget, sizeof(struct sis1100_irq_get));
        if (res!=sizeof(struct sis1100_irq_get)) {
            printf("read irq: res=%d %s\n", res, strerror(errno));
            return 1;    
        }

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

        /*irqget.irqs&=SIS3100_FRONT_IRQS;*/
        if (ioctl(p, SIS1100_IRQ_ACK, &irqget.irqs)<0) {
            printf("ioctl(SIS1100_IRQ_ACK): %s\n", strerror(errno));
            return 1;
        }
    }

    /* disable SIS3100 FRONT IRQS */
    irqctl.irq_mask=SIS3100_FRONT_IRQS;
    irqctl.signal=0;
    if (ioctl(p, SIS1100_IRQ_CTL, &irqctl)<0) {
        printf("ioctl(SIS1100_IRQ_CTL): %s\n", strerror(errno));
        return 1;    
    }

    /* deactivate all outputs */
    iobits=0xff<<16;
    if (ioctl(p, SIS1100_FRONT_IO, &iobits)<0) {
        printf("ioctl(SIS1100_FRONT_IO): %s\n", strerror(errno));
        return 1;    
    }

    close(p);
    return 0;    
}
/****************************************************************************/
/****************************************************************************/

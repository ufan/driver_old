#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "dev/pci/sis1100_var.h"

u_int32_t sis3801_base=0x10000000;

int irqlevel=3;
int irqvector=37;

/****************************************************************************/
static int
vme_read(int p, u_int32_t base, int addr, u_int32_t *val)
{
    int res;
    struct sis1100_vme_req req;

    req.size=4;
    req.am=0x9;
    req.addr=base+addr;

    if ((res=ioctl(p, SIS3100_VME_READ, &req))<0) {
        printf("vme read 0x%08x: errno=%s\n", req.addr, strerror(errno));
        return -1;
    }
    if (req.error) {
        printf("vme read 0x%08x: error=0x%x\n", req.addr, req.error);
        return -1;
    }
    *val=req.data;
    return 0;
}
/****************************************************************************/
static int
vme_write(int p, u_int32_t base, int addr, u_int32_t data)
{
    struct sis1100_vme_req req;

    req.size=4;
    req.am=0x9;
    req.addr=base+addr;
    req.data=data;
    if ((ioctl(p, SIS3100_VME_WRITE, &req))<0) {
        printf("vme write 0x%08x: errno=%s\n", req.addr, strerror(errno));
        return -1;
    }
    if (req.error) {
        printf("vme write 0x%08x: error=0x%x\n", req.addr, req.error);
        return -1;
    }
    return 0;
}
/****************************************************************************/
int main(int argc, char* argv[])
{
    int p, count=0;
    struct sis1100_irq_ctl irqctl;
    struct sis1100_irq_get irqget;
    struct sis1100_irq_ack irqack;

    if (argc!=2) {
        printf("usage: %s path\n", argv[0]);
        return 1;
    }

    if ((p=open(argv[1], O_RDWR, 0))<0) {
        printf("open %s: %s", argv[1], strerror(errno));
        return 1;
    }

    vme_write(p, sis3801_base, 0x60, 0); /* reset */

    irqctl.irq_mask=SIS3100_VME_IRQS;
    irqctl.signal=-1;
    if (ioctl(p, SIS1100_IRQ_CTL, &irqctl)<0) {
        printf("SIS1100_IRQ_CTL: %s\n", strerror(errno));
        return 1;    
    }
    irqack.irq_mask=0xffffffff;
    if (ioctl(p, SIS1100_IRQ_ACK, &irqack)<0) {
        printf("ioctl(SIS1100_IRQ_ACK): %s\n", strerror(errno));
        return 1;    
    }

    vme_write(p, sis3801_base, 0x20, 0); /* fifo clear */
    vme_write(p, sis3801_base, 0x28, 0); /* enable next logic */
    vme_write(p, sis3801_base, 0x0, 0x400000); /* enable irq source 2 (half full) */
    vme_write(p, sis3801_base, 0x0, 0x10000); /* enable external next */
    vme_write(p, sis3801_base, 0x4, 0x800|(irqlevel<<9)|irqvector);

    while (count++<10) {
        irqget.irq_mask=SIS3100_VME_IRQS;
        if (ioctl(p, SIS1100_IRQ_WAIT, &irqget)<0) {
            printf("ioctl(SIS1100_IRQ_WAIT): %s\n", strerror(errno));
            return 1;
        }

        printf("got irqs 0x%08x level=%d vector 0x%x\n",
                irqget.irqs, irqget.level, irqget.vector);

        if (!irqget.irqs) {
            printf("no irq bits set: impossible case\n");
            return 0;
        }

        {
            u_int32_t val, status;
            int count=0;
            while (1) {
                vme_read(p, sis3801_base, 0x0, &status);
                if (status&0x100)
                    break;
                vme_read(p, sis3801_base, 0x100, &val);
                count++;
            };
            printf("%d words read\n", count);
        }

        irqack.irq_mask=1<<irqget.level;
        if (ioctl(p, SIS1100_IRQ_ACK, &irqack)<0) {
            printf("ioctl(SIS1100_IRQ_ACK): %s\n", strerror(errno));
            return 1;    
        }
    }

    irqctl.irq_mask=SIS3100_VME_IRQS;
    irqctl.signal=0;
    if (ioctl(p, SIS1100_IRQ_CTL, &irqctl)<0) {
        printf("SIS1100_IRQ_CTL: %s\n", strerror(errno));
        return 1;    
    }

    close(p);
    return 0;    
}
/****************************************************************************/
/****************************************************************************/

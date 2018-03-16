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

int sis_base[]={0, 0x60000, 0x70000};

/****************************************************************************/
static int vme_read(int p, int base, int addr)
{
    int res;
    struct sis1100_vme_req req;

    req.size=4;
    req.am=0x9;
    req.addr=base+addr;
    res=ioctl(p, SIS3100_VME_READ, &req);
    if (res)
        printf("vme read 0x%08x: res=%s, error=0x%x\n",
    	    req.addr, strerror(errno), req.error);
    return req.data;
}
/****************************************************************************/
static void vme_write(int p, int base, int addr, int data)
{
    int res;
    struct sis1100_vme_req req;

    req.size=4;
    req.am=0x9;
    req.addr=base+addr;
    req.data=data;
    res=ioctl(p, SIS3100_VME_WRITE, &req);
    if (res)
        printf("vme write 0x%08x, 0x%08x: res=%s, error=0x%x\n",
    	    req.addr, req.data, strerror(errno), req.error);
}
/****************************************************************************/
volatile int idx, irq, irqcount=0;

static void sighnd(int sig)
{
    irq++; irqcount++;
    fprintf(stderr, "got sig %d\n", sig);
}
/****************************************************************************/
int main(int argc, char* argv[])
{
    int p, module, idx;
    struct sigaction action, old_action;
    struct sis1100_irq_ctl irqctl;
    struct sis1100_irq_get irqget;
    struct sis1100_irq_ack irqack;

    if (argc!=2)
        {
        fprintf(stderr, "usage: %s path\n", argv[0]);
        return 1;
        }

    if ((p=open(argv[1], O_RDWR, 0))<0) return 1;

    action.sa_handler=sighnd;
    sigemptyset(&action.sa_mask);
    action.sa_flags=0;
    sigaction(SIGUSR1, &action, &old_action);
    
/*
 *     sigemptyset(&mask);
 *     sigaddset(&mask, SIGUSR1);
 *     sigprocmask(SIG_BLOCK, &mask, &old_mask);
 */

    for (module=0; module<3; module++) {
        vme_write(p, sis_base[module], 0x60, 0); /* reset */
    }

    irqctl.irq_mask=0xffff; /* ALL levels; just for fun */
    irqctl.signal=SIGUSR1;
    if (ioctl(p, SIS1100_IRQ_CTL, &irqctl)<0) {
        fprintf(stderr, "ioctl(SIS3100_IRQ_CTL): %s\n", strerror(errno));
        return 1;    
    }

    idx=0;
    for (module=0; module<3; module++) {
        int base, level;
        base=sis_base[module];
        level=module+1;
        vme_write(p, base, 0x20, 0); /* fifo clear */
        vme_write(p, base, 0x28, 0); /* enable next logic */
        vme_write(p, base, 0x0, 0x400000); /* enable irq source 2 (half full) */
        vme_write(p, base, 0x4, 0x800|((level&7)<<9)|level);

        irq=0;
        while (!irq) {
            idx++;
            vme_write(p, base, 0x24, 0); /* clock */
        }    
    }

/*
 *     sigsuspend(&old_mask);
 *     fprintf(stderr, "after suspend\n");
 */

    while (1) {
        int base, count, data;
        irqget.irq_mask=0xffff;
        irqget.immediate_ack=0;
        if (ioctl(p, SIS1100_IRQ_GET, &irqget)<0) {
            fprintf(stderr, "ioctl(SIS1100_IRQ_GET): %s\n", strerror(errno));
            return 1;    
        }
        printf("got irqs 0x%08x level=%d vector 0x%x\n",
                irqget.irqs, irqget.level, irqget.vector);
        if (!irqget.irqs) return 0;

        base=sis_base[irqget.vector-1];
        count=0;
        while ((vme_read(p, base, 0x0)&0x100)==0) {
            data=vme_read(p, base, 0x100);
            count++;
        }
        printf("count=%d\n", count);

        irqack.irq_mask=1<<irqget.level;
        if (ioctl(p, SIS1100_IRQ_ACK, &irqack)<0) {
            fprintf(stderr, "ioctl(SIS1100_IRQ_ACK): %s\n", strerror(errno));
            return 1;    
        }
    }
    sigaction(SIGUSR1, &old_action, 0);

    close(p);
    return 0;    
}
/****************************************************************************/
/****************************************************************************/

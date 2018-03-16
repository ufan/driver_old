#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "dev/pci/sis1100_var.h"

int ngf_base=0xe00000;

#define SFI_W(info, x, v) (((sfi_w)(info)->base)->x=H2SFI(v))
#define SEQ_W(info, x, v) SFI_W(info, seq[x], v)

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
static void ngf_status(int p)
{
    printf("===============\n");
    printf("[2020] = %04X\n", vme_read(p, ngf_base, 0x2020)&0xffff);
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
    int p;
    struct sigaction action, old_action;
    struct sis1100_irq_ctl irqctl;
    struct sis1100_irq_get irqget;
    struct sis1100_irq_ack irqack;
    sigset_t mask, old_mask;

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
    
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);

    irqctl.irq_mask=0xffff; /* ALL irq_mask; just for fun */
    irqctl.signal=SIGUSR1;
    if (ioctl(p, SIS1100_IRQ_CTL, &irqctl)<0) {
        fprintf(stderr, "ioctl(SIS3100_IRQ_CTL): %s\n", strerror(errno));
        return 1;    
    }

    vme_write(p, ngf_base, 0x201c, 0); /* reset */
    vme_write(p, ngf_base, 0x2030, 0); /* reset sequencer */
    vme_write(p, ngf_base, 0x2038, 0); /* clear seq. command flag */
    ngf_status(p);
    vme_write(p, ngf_base, 0x2020, 0); /* start sequencer */
    vme_write(p, ngf_base, 0x2010, 0x977); /* irq level */
    vme_write(p, ngf_base, 0x2014, 0xff); /* irq mask */
    ngf_status(p);
    vme_write(p, ngf_base, 0x10000+0x68, 0); /* set seq. command flag */
    ngf_status(p);

    sigsuspend(&old_mask);
    fprintf(stderr, "after suspend\n");

    irqget.irq_mask=0xffff;
    irqget.immediate_ack=0;
    if (ioctl(p, SIS1100_IRQ_GET, &irqget)<0) {
        fprintf(stderr, "ioctl(SIS3100_IRQ_GET): %s\n", strerror(errno));
        return 1;    
    }
    printf("got level 0x%x vector 0x%x\n", irqget.irq_mask, irqget.vector);

    vme_write(p, ngf_base, 0x2038, 0); /* clear seq. command flag */
    ngf_status(p);

    irqack.irq_mask=irqget.irq_mask;
    if (ioctl(p, SIS1100_IRQ_ACK, &irqack)<0) {
        fprintf(stderr, "ioctl(SIS3100_IRQ_ACK): %s\n", strerror(errno));
        return 1;    
    }
    sigaction(SIGUSR1, &old_action, 0);

    close(p);
    return 0;    
}
/****************************************************************************/
/****************************************************************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "../dev/pci/sis1100_var.h"

/* basic setup of SIS3300 */
static const u_int32_t base=0x80000000;
static const int irq_level=5;
//static const int irq_mode=0; /* RORA */
static const int irq_mode=1; /* ROAK */
static const int irq_vector=0x5a;

volatile int sig;

/****************************************************************************/
static void
sighnd(int sig_)
{
#if 1
    printf("got signal %d\n", sig_);
#endif
    sig=sig_;
}
/*****************************************************************************/
static int
set_vmespace(int p, int datasize, int am, int mindmalen, int fifomode)
{
    struct vmespace vspace;

    vspace.datasize=datasize;
    vspace.am=am;
    vspace.swap = 0;
    vspace.mindmalen=mindmalen;
    vspace.mapit = 0;
#if 0
    printf("vmespace: p=%d size=%d am=%x mindmalen=%d fifo=%d\n",
        p, datasize, am, mindmalen, fifomode);
#endif
    if (ioctl(p, SETVMESPACE, &vspace)<0) {
        printf("SETVMESPACE: %s\n", strerror(errno));
        return -1;
    }
    if (ioctl(p, SIS1100_FIFOMODE, &fifomode)<0) {
        printf("FIFOMODE: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}
/****************************************************************************/
static int
vmeread(int p, u_int32_t offs, int size, u_int32_t *val)
{
    if (pread(p, val, size, base+offs)!=size) {
        printf("vmeread(0x%x, %d): %s\n",
                offs, size, strerror(errno));
        return -1;
    }
    return 0;
}
/****************************************************************************/
static int
vmewrite(int p, u_int32_t offs, int size, u_int32_t val)
{
    if (pwrite(p, &val, size, base+offs)!=size) {
        printf("vmewrite(0x%x, %d, 0x%x): %s\n",
                offs, size, val, strerror(errno));
        return -1;
    }
    return 0;
}
/****************************************************************************/
int
main(int argc, char* argv[])
{
    int p;
    struct sigaction action;
    struct sis1100_irq_ctl irqctl;
    sigset_t mask, old_mask;
    u_int32_t val;

    if (argc!=2) {
        printf("usage: %s path\n", argv[0]);
        return 1;
    }

    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);

    if ((p=open(argv[1], O_RDWR, 0))<0) {
        printf("open %s: %s\n", argv[1], strerror(errno));
        return 2;
    }

    action.sa_handler=sighnd;
    sigemptyset(&action.sa_mask);
    action.sa_flags=0;
    sigaction(SIGUSR1, &action, 0);

    irqctl.irq_mask=0xffffffff;
    irqctl.signal=SIGUSR1;
    if (ioctl(p, SIS1100_IRQ_CTL, &irqctl)<0) {
        printf("ioctl(SIS1100_IRQ_CTL): %s\n", strerror(errno));
        return 3;    
    }

    set_vmespace(p, 4, 9, 0, 0);
    /* disable all IRQ sources */
    vmewrite(p, 0xc, 4, 0xf<<16);
    /* write IRQ configuration */
    vmewrite(p, 0x8, 4, (irq_mode<<12)|(1<<11)|(irq_level<<8)|irq_vector);
    /* enable IRQ source 3 (user input) */
    vmewrite(p, 0xc, 4, 0x8);

#if 0
    val=2;
    if (ioctl(p, SIS1100_TESTFLAGS, &val)<0) {
        printf("ioctl(SIS1100_TESTFLAGS): %s\n", strerror(errno));
        return 4;
    }
#endif
#if 0
    val=irq_level;
    if (ioctl(p, SIS1100_IRQ_READ_VECTOR, &val)<0) {
        printf("ioctl(IRQ_READ_VECTOR): %s\n", strerror(errno));
        return 5;
    }
    printf("Vector is 0x%x\n", val);
#endif

    while(1) {
        struct sis1100_irq_extget irqget;
        struct sis1100_irq_ack irqack;
        u_int32_t val;
        printf("schlafe ein\n");
        sigsuspend(&old_mask);
        printf("aufgewacht\n");

#if 1
        val=irq_level;
        if (ioctl(p, SIS1100_IRQ_READ_VECTOR, &val)<0) {
            printf("ioctl(IRQ_READ_VECTOR): %s\n", strerror(errno));
            continue;
        }
        printf("Vector is 0x%x\n", val);
#endif

        irqget.irq_mask=0xffffffff;
        irqget.flags=0x0;
        if (ioctl(p, SIS1100_IRQ_EXTGET, &irqget)<0) {
            printf("ioctl(SIS1100_IRQ_EXTGET): %s\n", strerror(errno));
            break;
        }
        printf("got:\n");
        printf("    mask  : 0x%08x\n", irqget.irq_mask);
        printf("    status: %d\n", irqget.remote_status);
        printf("    opt   : 0x%08x\n", irqget.opt_status);
        printf("    mbx0  : 0x%08x\n", irqget.mbx0);
        printf("    irqs  : 0x%08x\n", irqget.irqs);
        printf("    level : %d\n", irqget.level);
        printf("    vector: 0x%02x\n", irqget.vector);
        printf("    sec   : %d.%06d\n",
                irqget.irq_sec&0xffffffff, irqget.irq_nsec/1000);

        vmeread(p, 0x8, 4, &val);
        printf("  0x8: 0x%08x\n", val);
        vmeread(p, 0xc, 4, &val);
        printf("  0xc: 0x%08x\n", val);

        irqack.irq_mask=irqget.irqs;
        if (ioctl(p, SIS1100_IRQ_ACK, &irqack)<0) {
            printf("ioctl(SIS1100_IRQ_ACK): %s\n", strerror(errno));
            break;
        }

        /* disable and reenable IRQ source */
        vmewrite(p, 0xc, 4, 0x8<<16);
        vmewrite(p, 0xc, 4, 0x8);
    }

    /* disable all IRQ sources */
    vmewrite(p, 0xc, 4, 0xf<<16);
    /* disable IRQs */
    vmewrite(p, 0x8, 4, 0);

    close(p);
}

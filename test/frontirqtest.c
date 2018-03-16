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
static void sighnd(int sig)
{
    fprintf(stderr, "got sig %d\n", sig);
}
/****************************************************************************/
int main(int argc, char* argv[])
{
    int p, front_io;
    struct sigaction action;
    struct sis1100_irq_ctl irqctl;
    struct sis1100_irq_get irqget;
    struct sis1100_irq_ack irqack;
    sigset_t mask, old_mask;
    struct sis1100_ctrl_reg reg;

    if (argc!=2)
        {
        fprintf(stderr, "usage: %s path\n", argv[0]);
        return 1;
        }

    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);

    if ((p=open(argv[1], O_RDWR, 0))<0) return 1;

    front_io=0x7f;
    ioctl(p, SIS1100_FRONT_IO, &front_io);

    action.sa_handler=sighnd;
    sigemptyset(&action.sa_mask);
    action.sa_flags=0;
    sigaction(SIGUSR1, &action, 0);

    irqctl.irq_mask=0xffffffff; /* all IRQs */
    irqctl.signal=SIGUSR1;
    if (ioctl(p, SIS1100_IRQ_CTL, &irqctl)<0) {
        fprintf(stderr, "ioctl(SIS1100_IRQ_CTL): %s\n", strerror(errno));
        return 1;    
    }

reg.offset=0x8;
ioctl(p, SIS1100_CONTROL_READ, &reg);
printf("0x%08x\n", reg.val);

    while (1) {
        u_int32_t io_bits;

        sigsuspend(&old_mask);

        irqget.irq_mask=0xffffffff;
        irqget.immediate_ack=0;
        if (ioctl(p, SIS1100_IRQ_GET, &irqget)<0) {
            fprintf(stderr, "ioctl(SIS1100_IRQ_GET): %s\n", strerror(errno));
            return 1;
        }
        printf("irqget: 0x%08x\n", irqget.irqs);
        switch (irqget.remote_status) {
            case -1:
                printf("Link down\n");
                io_bits=(3<<26) | (1<<23);
                ioctl(p, SIS1100_FRONT_IO, &io_bits);
                break;
            case 1:
                printf("Link up\n");
                io_bits=(3<<10) | (1<<7);
                ioctl(p, SIS1100_FRONT_IO, &io_bits);
                break;
        }
        irqack.irq_mask=irqget.irqs;
        if (ioctl(p, SIS1100_IRQ_ACK, &irqack)<0) {
            fprintf(stderr, "ioctl(SIS1100_IRQ_ACK): %s\n", strerror(errno));
            return 1;
        }
    }

    close(p);
    return 0;    
}
/****************************************************************************/
/****************************************************************************/

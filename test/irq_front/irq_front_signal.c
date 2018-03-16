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

#include <sis1100_var.h>

volatile int sig;

/****************************************************************************/
static void
sighnd(int sig_)
{
#if 0
    printf("got signal %d\n", sig_);
#endif
    sig=sig_;
}
/****************************************************************************/
int
main(int argc, char* argv[])
{
    int p;
    struct sigaction action;
    struct sis1100_irq_ctl irqctl;
    struct sis1100_irq_get irqget;
    struct sis1100_irq_ack irqack;
    sigset_t mask, old_mask;

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

    irqctl.irq_mask=0;
    irqctl.signal=SIGUSR1;
    if (ioctl(p, SIS1100_IRQ_CTL, &irqctl)<0) {
        printf("ioctl(SIS1100_IRQ_CTL): %s\n", strerror(errno));
        return 3;    
    }

    while (1) {
        u_int32_t io_bits;

        sigsuspend(&old_mask);
        if (!sig) {
            printf("alien signal received\n");
            continue;
        }
        sig=0;

        irqget.irq_mask=0;
        if (ioctl(p, SIS1100_IRQ_GET, &irqget)<0) {
            printf("ioctl(SIS1100_IRQ_GET): %s\n", strerror(errno));
            return 1;
        }
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
            default:
                printf("got irq, but link state is unchanged\n");
        }
#if 0
/* IRQ_ACK is not necessary inn case of link IRQ */
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

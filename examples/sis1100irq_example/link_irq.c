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

/*****************************************************************************/
static const char *hwnames[]={
    "invalid",
    "PCI",
    "VME",
    "CAMAC",
    "FZJ-LVDS-System"
};

static void
sis1100_print_ident(int p)
{
    struct sis1100_ident ident;

    if (ioctl(p, SIS1100_IDENT, &ident)<0) {
        printf("sis1100_print_ident: fcntl(IDENT): %s\n", strerror(errno));
        return;
    }
    printf("sis1100 local ident:\n");
    printf("  hw_type    = %d\n", ident.local.hw_type);
    printf("  hw_version = %d\n", ident.local.hw_version);
    printf("  fw_type    = %d\n", ident.local.fw_type);
    printf("  fw_version = %d\n", ident.local.fw_version);
    if (ident.remote_ok) {
        printf("sis1100 remote ident:\n");
        printf("  hw_type    = %d\n", ident.remote.hw_type);
        printf("  hw_version = %d\n", ident.remote.hw_version);
        printf("  fw_type    = %d\n", ident.remote.fw_type);
        printf("  fw_version = %d\n", ident.remote.fw_version);
        if (ident.remote.hw_type<sizeof(hwnames)/sizeof(const char*)) {
            printf("remote side is %s\n", hwnames[ident.remote.hw_type]);
        } else {
            printf("remote side is of unknown type %d\n", ident.remote.hw_type);
        }
        printf("remote side is O%sLINE\n", ident.remote_online?"N":"FF");
    } else {
        printf("remote side is OFFLINE\n");
    }
}
/****************************************************************************/
static void sighnd(int sig)
{
    printf("got sig %d\n", sig);
}
/****************************************************************************/
int main(int argc, char* argv[])
{
    int p, count=0;
    struct sigaction action, oldaction;
    struct sis1100_irq_ctl irqctl;
    struct sis1100_irq_get irqget;
    sigset_t mask, old_mask;

    if (argc!=2) {
        printf("usage: %s path\n", argv[0]);
        return 1;
    }

    if ((p=open(argv[1], O_RDWR, 0))<0)
        return 1;

    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);

    action.sa_handler=sighnd;
    sigemptyset(&action.sa_mask);
    action.sa_flags=0;
    sigaction(SIGUSR1, &action, &oldaction);

    irqctl.irq_mask=0;
    irqctl.signal=SIGUSR1;
    if (ioctl(p, SIS1100_IRQ_CTL, &irqctl)<0) {
        printf("ioctl(SIS1100_IRQ_CTL): %s\n", strerror(errno));
        return 1;    
    }

    while (count++<10) {
        sigsuspend(&old_mask);

        irqget.irq_mask=0;
        if (ioctl(p, SIS1100_IRQ_GET, &irqget)<0) {
            printf("ioctl(SIS1100_IRQ_GET): %s\n", strerror(errno));
            return 1;
        }
        printf("irqs=%08x, remote_status=%d\n",
                irqget.irqs, irqget.remote_status);
        if (irqget.remote_status!=0) {
            sis1100_print_ident(p);
        }

        /* in this case no acknowledge necessary */
    }

    irqctl.irq_mask=0;
    irqctl.signal=0;
    if (ioctl(p, SIS1100_IRQ_CTL, &irqctl)<0) {
        printf("ioctl(SIS1100_IRQ_CTL): %s\n", strerror(errno));
        return 1;    
    }

    sigaction(SIGUSR1, &oldaction, 0);

    close(p);
    return 0;    
}
/****************************************************************************/
/****************************************************************************/

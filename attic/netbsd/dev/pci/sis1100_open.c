/* $ZEL: sis1100_open.c,v 1.5 2003/01/15 14:17:01 wuestner Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <vm/vm.h>
#include <uvm/uvm_extern.h>
#include <sys/malloc.h>
#include <sys/lock.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/pci/sis1100_var.h>
#include <dev/pci/sis1100_sc.h>

int
sis1100_open(dev_t dev, int flag, int mode, struct proc *p)
{
    struct SIS1100_softc* sc;
    struct SIS1100_fdata* fd;
    unsigned int minor=minor(dev);
    unsigned int card=(minor&sis1100_MINORCARDMASK)>>sis1100_MINORCARDSHIFT;
    unsigned int subdev=(minor&sis1100_MINORTYPEMASK)>>sis1100_MINORTYPESHIFT;
    unsigned int idx=minor&(sis1100_MINORUTMASK);

    if (card >= sis1100cfdriver.cd_ndevs || !sis1100cfdriver.cd_devs[card]) {
        printf("sis1100 open: returning ENXIO\n");
        return (ENXIO);
    }
    sc=SIS1100SC(dev);

    if (sc->fdatalist[idx]) {
        printf("sis1100 open: fdatalist[%d]=%p\n", idx, sc->fdatalist[idx]);
        printf("sis1100 open: returning EBUSY\n");
        return EBUSY;
    }
    fd=malloc(sizeof(struct SIS1100_fdata), M_DEVBUF, M_WAITOK);
    fd->subdev=subdev;
    fd->p=p;
    fd->pid=p->p_pid;
/*
    printf("%s: open: pid=%d card=%d, subdev=%d, idx=%d\n",
        sc->sc_dev.dv_xname,
        fd->pid,
        card,
        fd->subdev,
        idx);
*/
    fd->big_endian=1;
    fd->fifo_mode=0;          /* can't be changed for sdram and sharc */
    fd->vmespace_am=9;        /* useless for sdram and sharc and control */
    fd->vmespace_datasize=4;  /* useless for sdram and sharc and control */
    fd->last_prot_err=0;
    fd->mindmalen_r=50;
    fd->mindmalen_w=50;
    fd->sig=0;
    fd->owned_irqs=0;         /* useless for sdram and sharc */
    fd->mmapdma.valid=0;
    simple_lock(&sc->lock_sc_inuse);
    sc->sc_inuse++;
    simple_unlock(&sc->sc_inuse);
    sc->fdatalist[idx]=fd;
    return 0;
}

int
sis1100_close(dev_t dev, int flag, int mode, struct proc *p)
{
        struct SIS1100_softc* sc=SIS1100SC(dev);
        struct SIS1100_fdata* fd=SIS1100FD(dev);
        unsigned int minor=minor(dev);
        unsigned int idx=minor&(sis1100_MINORUTMASK);

        if (fd->mmapdma.valid) dma_free(sc, fd, 0);
        if (fd) {
            sc->fdatalist[idx]=0;

            if (fd->owned_irqs & SIS3100_IRQS)
                    sis3100writeremreg(sc, vme_irq_sc,
                            (fd->owned_irqs & SIS3100_IRQS)<<16, 0);

            free(fd, M_DEVBUF);
        }
        simple_lock(&sc->lock_sc_inuse);
	sc->sc_inuse--;
        simple_unlock(&sc->sc_inuse);
        return 0;
}

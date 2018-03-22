/* $ZEL: sis1100_vme_irq.c,v 1.3 2003/01/15 14:17:03 wuestner Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/select.h>
#include <sys/malloc.h>
#include <sys/callout.h>
#include <sys/kthread.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/sis1100_var.h>
#include <dev/pci/sis1100_sc.h>

static int
sis1100_irq_acknowledge(struct SIS1100_softc* sc, int level)
{
    int vector;
    u_int32_t error;

    lockmgr(&sc->sem_hw, LK_EXCLUSIVE, 0);
    if (level&1)
        sis1100writereg(sc, t_hdr, 0x0c010802);
    else
        sis1100writereg(sc, t_hdr, 0x03010802);
    sis1100writereg(sc, t_am, (1<<14)|0x3f);
    sis1100writereg(sc, t_adl, level<<1);
    error=sis1100readreg(sc, prot_error);

    if (error) {
	printf("SIS1100: error in Iack level %d: 0x%x\n",
                level, error);
        vector=-1;
    } else {
        vector=sis1100readreg(sc, tc_dal)&0xff;
    }
    lockmgr(&sc->sem_hw, LK_RELEASE, 0);
    return vector;
}

void
sis1100thread_vmeirq(void* data)
{
    struct SIS1100_softc* sc=(struct SIS1100_softc*)data;
    int command;

    printf("sis1100thread_vmeirq started\n");
    while (1) {
        tsleep(&sc->vmeirq_pp, PCATCH, "thread_vmeirq", 0);
        command=sc->vmeirq_command.command;
        if (command<0) {
            printf("sis1100thread_vmeirq terminated\n");
            simple_lock(&sc->vmeirq_command.lock);
            sc->vmeirq_command.command=0;
            wakeup(sc);
            simple_unlock(&sc->vmeirq_command.lock);
            kthread_exit(0);
        }
        printf("sis1100thread_vmeirq aufgewacht\n");
        sis1100_vme_irq_handler(sc);
    }
}

void
sis1100_vme_irq_handler(void* data)
{
    struct SIS1100_softc* sc=(struct SIS1100_softc*)data;
    int new_irqs, i, idx, s;

    new_irqs=sc->doorbell&~sc->pending_irqs;
    sc->doorbell=0;
    s=splbio();
    simple_lock(&sc->vmeirq_wait);
    sc->pending_irqs|=new_irqs;
    simple_unlock(&sc->vmeirq_wait);
    splx(s);

    /* block IRQs in VME controller*/
    if (new_irqs & SIS3100_VME_IRQS) {
        sis3100writeremreg(sc, vme_irq_sc, (new_irqs&SIS3100_VME_IRQS)<<16, 1);
        sis3100writeremreg(sc, vme_irq_sc, 1<<15, 1);
    }
    /* obtain irq vectors from VME */
    for (i=7; i>0; i--) {
        if (new_irqs & (1<<i)) {
            sc->irq_vects[i].vector=sis1100_irq_acknowledge(sc, i);
            sc->irq_vects[i].valid=1;
            /*printf("vme_irq_handler: level %d vector=0x%08x\n",
                i, sc->irq_info[i].vector);*/
        }
    }
    /* block FRONT-IRQs in VME controller*/
    if (new_irqs & SIS3100_EXT_IRQS) {
        sis3100writeremreg(sc, in_latch_irq, (new_irqs & SIS3100_EXT_IRQS)<<16, 1);
        sis3100writeremreg(sc, in_latch_irq, 1<<15, 1);
    }

    for (idx=0; idx<=sis1100_MINORUTMASK; idx++) {
        struct SIS1100_fdata* fd=sc->fdatalist[idx];

        if (fd->sig && (fd->sig!=-1) &&
                ((new_irqs & fd->owned_irqs)||
                        (sc->old_remote_hw!=sc->remote_hw))) {
            psignal(fd->p, fd->sig);
        }
    }

    sc->old_remote_hw=sc->remote_hw;
    wakeup(&sc->vmeirq_wait);
}

int
sis1100_irq_ctl(struct SIS1100_softc* sc, struct SIS1100_fdata* fd,
    struct sis1100_irq_ctl* data)
{
        int foreign_irqs;
        int idx;

        printf("%s irq_ctl:  mask=0x%08x, signal=%d\n",
                        sc->sc_dev.dv_xname, data->irq_mask, data->signal);
        if (data->signal) {
                foreign_irqs=0;
                /* irq already in use? */
                for (idx=0; idx<=sis1100_MINORUTMASK; idx++) {
                    foreign_irqs |= sc->fdatalist[idx]->owned_irqs;
                }
                if (foreign_irqs & data->irq_mask) {
                        printf("%s irq_ctl: "
                                "IRQs owned by other programs: 0x%08x\n", 
                                sc->sc_dev.dv_xname, foreign_irqs);
                        return  -EBUSY;
                }

                fd->sig=data->signal;
                fd->owned_irqs |= data->irq_mask;
                fd->old_remote_hw=sc->remote_hw;

/* XXX clear pending IRQs? */
/* XXX use sis1100_irq.c::sis1100_enable_irq? */
                /* enable VME-IRQs */
                if (data->irq_mask & SIS3100_VME_IRQS) {
                        u_int32_t mask;
                        mask=data->irq_mask & SIS3100_IRQS;
                        sis3100writeremreg(sc, vme_irq_sc, mask, 0);
                }
                /* enable VME-FRONT-IRQs */
                if (data->irq_mask & SIS3100_EXT_IRQS) {
                        u_int32_t mask;
                        mask=(data->irq_mask & SIS3100_EXT_IRQS)>>8;
                        sis3100writeremreg(sc, in_latch_irq, mask, 0);
                }
                /* enable PCI-FRONT-IRQs */
                if (data->irq_mask & SIS1100_FRONT_IRQS) {
                        u_int32_t mask;
                        mask=(data->irq_mask & SIS1100_FRONT_IRQS)>>4;
                        sis1100writereg(sc, cr, mask);
                }
        } else {
                int irqs;

                irqs=fd->owned_irqs & data->irq_mask;

                if (data->irq_mask & SIS3100_VME_IRQS) {
                        u_int32_t mask;
                        mask=(irqs & SIS3100_IRQS)<<16;
                        sis3100writeremreg(sc, vme_irq_sc, mask, 0);
                }
                if (data->irq_mask & SIS3100_EXT_IRQS) {
                        u_int32_t mask;
                        mask=(irqs & SIS3100_EXT_IRQS)<<8;
                        sis3100writeremreg(sc, in_latch_irq, mask, 0);
                }
                if (data->irq_mask & SIS1100_FRONT_IRQS) {
                        u_int32_t mask;
                        mask=(irqs & SIS1100_FRONT_IRQS)<<12;
                        sis1100writereg(sc, cr, mask);
                }

                fd->owned_irqs &= ~data->irq_mask;
        }
        return 0;
}

static void
_sis1100_irq_get_vector(struct SIS1100_softc* sc, int irqs,
                        struct sis1100_irq_get* data)
{
        if (irqs & SIS3100_VME_IRQS) {
                int bit;
                /* find highest bit set */
                for (bit=7; bit>0; bit--) {
                        if (((1<<bit) & irqs) && sc->irq_vects[bit].valid) {
                                data->level=bit;
                                data->vector=sc->irq_vects[bit].vector;
                                sc->irq_vects[bit].valid=0;
                                break;
                        }
                }
        } else {
                data->vector=-1;
                data->level=0;
        }
}

static void
_sis1100_irq_ack(struct SIS1100_softc* sc, int irqs)
{
        sc->pending_irqs&=~irqs;

        if (irqs & SIS3100_VME_IRQS)
                sis3100writeremreg(sc, vme_irq_sc, irqs & SIS3100_VME_IRQS, 0);
        if (irqs & SIS3100_EXT_IRQS)
                sis3100writeremreg(sc, in_latch_irq, (irqs&SIS3100_EXT_IRQS)>>8, 0);
}

int
sis1100_irq_ack(struct SIS1100_softc* sc, struct SIS1100_fdata* fd,
    struct sis1100_irq_ack* data)
{
        int irqs;

        irqs=fd->owned_irqs & data->irq_mask & sc->pending_irqs;

        _sis1100_irq_ack(sc, irqs);
        return 0;
}

int
sis1100_irq_get(struct SIS1100_softc* sc, struct SIS1100_fdata* fd,
    struct sis1100_irq_get* data)
{
        data->irqs=sc->pending_irqs & fd->owned_irqs;
        if (fd->old_remote_hw!=sc->remote_hw) {
                if (sc->remote_hw)
                        data->remote_status=1;
                else
                        data->remote_status=-1;
                fd->old_remote_hw=sc->remote_hw;
        } else
                data->remote_status=0;

        if (sc->remote_hw==sis1100_hw_vme)
            _sis1100_irq_get_vector(sc, data->irqs & data->irq_mask, data);
        else {
            data->level=0;
            data->vector=0;
        }
        return 0;
}

int
sis1100_irq_wait(struct SIS1100_softc* sc, struct SIS1100_fdata* fd,
    struct sis1100_irq_get* data)
{
        int irqs, res=0, s;

        irqs=fd->owned_irqs & data->irq_mask;

        s=splbio();
        simple_lock(&sc->vmeirq_wait);
        while (!(sc->pending_irqs & irqs) &&
                        (fd->old_remote_hw==sc->remote_hw)) {
            res=ltsleep(&sc->vmeirq_wait, PCATCH, "vmeirq", 0, &sc->vmeirq_wait);
        }
        simple_unlock(&sc->vmeirq_wait);
        splx(s);
        if (res) return -EINTR;

        data->irqs=sc->pending_irqs & fd->owned_irqs;
        if (fd->old_remote_hw!=sc->remote_hw) {
                if (sc->remote_hw)
                        data->remote_status=1;
                else
                        data->remote_status=-1;
                fd->old_remote_hw=sc->remote_hw;
        } else
                data->remote_status=0;

        _sis1100_irq_get_vector(sc, data->irqs & data->irq_mask, data);

        return 0;
}

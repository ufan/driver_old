/* $ZEL: sis1100_vme_irq.c,v 1.9 2002/08/12 10:36:40 wuestner Exp $ */

#include "Copyright"

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/wrapper.h>
#include <linux/pci.h>
#include <linux/list.h>
#include <asm/uaccess.h>

#include <dev/pci/sis1100_sc.h>

static int
sis1100_irq_acknowledge(struct SIS1100_softc* sc, int level)
{
    int vector;
    u_int32_t error;

    down(&sc->sem_hw);
    if (level&1)
        sis1100writereg(sc, t_hdr, 0x0c010802);
    else
        sis1100writereg(sc, t_hdr, 0x03010802);
    sis1100writereg(sc, t_am, (1<<14)|0x3f);
    sis1100writereg(sc, t_adl, level<<1);
    error=sis1100readreg(sc, prot_error);

    if (error) {
	printk(KERN_ERR "SIS1100: error in Iack level %d: 0x%x\n",
                level, error);
        vector=-1;
    } else {
        vector=sis1100readreg(sc, tc_dal)&0xff;
    }
    up(&sc->sem_hw);
    return vector;
}

void
sis1100_vme_irq_handler(void* data)
{
    struct SIS1100_softc* sc=(struct SIS1100_softc*)data;
    int new_irqs, i;
    struct list_head* curr;
    int linkstatus;
/*
    printk(KERN_INFO "SIS1100[%d] vme_irq_handler: doorbell=0x%08x\n",
                sc->unit, sc->doorbell);
*/
    linkstatus=(sc->remote_ok+1)<<18;

    new_irqs=sc->doorbell&~sc->pending_irqs;
    sc->doorbell=0;
    sc->pending_irqs|=new_irqs;

    down(&sc->sem_fdata_list);
    /* block IRQs in VME controller*/
    if (new_irqs & SIS3100_VME_IRQS) {
        sis3100writereg(sc, vme_irq_sc, (new_irqs&SIS3100_VME_IRQS)<<16, 1);
    }
    /* obtain irq vectors from VME */
    for (i=7; i>0; i--) {
        if (new_irqs & (1<<i)) {
            sc->irq_vects[i].vector=sis1100_irq_acknowledge(sc, i);
            sc->irq_vects[i].valid=1;
            /*printk(KERN_INFO "vme_irq_handler: level %d vector=0x%08x\n",
                i, sc->irq_info[i].vector);*/
        }
    }
    /* block and clear FRONT-IRQs in VME controller*/
    if (new_irqs & SIS3100_EXT_IRQS) {
        sis3100writereg(sc, in_latch_irq, (new_irqs&SIS3100_EXT_IRQS)<<8, 1);
        sis3100writereg(sc, in_latch_irq, (new_irqs&SIS3100_EXT_IRQS)<<16, 1);
    }
/*
    printk(KERN_INFO "SIS1100[%d]: new_irqs=%08x\n", sc->unit, new_irqs);
*/
    list_for_each(curr, &sc->fdata_list_head) {
        struct SIS1100_fdata* fd;
        fd=list_entry(curr, struct SIS1100_fdata, list);
/*
        printk(KERN_INFO "SIS1100[%d]: pid %d, owned_irqs=%08x, sig=%d\n",
                sc->unit, fd->pid, fd->owned_irqs, fd->sig);
*/
        if (fd->sig && (fd->sig!=-1) &&
                ((new_irqs & fd->owned_irqs)||
                        (sc->old_remote_ok!=sc->remote_ok))) {
            int res;
/*
            printk(KERN_INFO "SIS1100[%d]: send sig %d to pid %d\n",
                sc->unit, fd->pid, fd->sig);
*/
            res=kill_proc_info(fd->sig, (void*)0, fd->pid);
            if (res)
                printk(KERN_WARNING "SIS1100[%d] : send sig %d to %d: res=%d\n",
                    sc->unit, fd->sig, fd->pid, res);
        }
    }

    sis3100writereg(sc, vme_irq_sc, 1<<15, 1);
    /*sis3100writereg(sc, in_latch_irq, 1<<15, 1);*/
    up(&sc->sem_fdata_list);
    sc->old_remote_ok=sc->remote_ok;
    wake_up_interruptible(&sc->irq_wait);
}

int
sis1100_irq_ctl(struct SIS1100_fdata* fd, struct sis1100_irq_ctl* data)
{
        struct SIS1100_softc* sc=fd->sc;
        int foreign_irqs;
        struct list_head* curr;

        if (data->signal) {
                foreign_irqs=0;
                down(&sc->sem_fdata_list);
                /* irq already in use? */
                list_for_each(curr, &sc->fdata_list_head) {
                        struct SIS1100_fdata* fd;
                        fd=list_entry(curr, struct SIS1100_fdata, list);
                        foreign_irqs |= fd->owned_irqs;
                }
                up(&sc->sem_fdata_list);
                if (foreign_irqs & data->irq_mask) {
                        printk(KERN_INFO "SIS1100[%d] irq_ctl: "
                                "IRQs owned by other programs: 0x%08x\n", 
                                sc->unit, foreign_irqs);
                        return  -EBUSY;
                }

                fd->pid=current->pid;
                fd->sig=data->signal;
                fd->owned_irqs |= data->irq_mask;
                fd->old_remote_ok=sc->remote_ok;

/* XXX clear pending IRQs? */
/* XXX use sis1100_irq.c::sis1100_enable_irq? */
                /* enable VME-IRQs */
                if (data->irq_mask & SIS3100_VME_IRQS) {
                        u_int32_t mask;
                        mask=data->irq_mask & SIS3100_IRQS;
                        sis3100writereg(sc, vme_irq_sc, mask, 0);
                }
                /* enable VME-FRONT-IRQs */
                if (data->irq_mask & SIS3100_EXT_IRQS) {
                        u_int32_t mask;
                        mask=data->irq_mask & SIS3100_EXT_IRQS;
                        sis3100writereg(sc, in_latch_irq, mask<<16, 1);
                        sis3100writereg(sc, in_latch_irq, mask>>8, 0);
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
                        sis3100writereg(sc, vme_irq_sc, mask, 0);
                }
                if (data->irq_mask & SIS3100_EXT_IRQS) {
                        u_int32_t mask;
                        mask=(irqs & SIS3100_EXT_IRQS)<<8;
                        sis3100writereg(sc, in_latch_irq, mask, 0);
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
                sis3100writereg(sc, vme_irq_sc, irqs & SIS3100_VME_IRQS, 0);

        if (irqs & SIS3100_EXT_IRQS)
                sis3100writereg(sc, in_latch_irq, (irqs&SIS3100_EXT_IRQS)>>8, 0);
}

int
sis1100_irq_ack(struct SIS1100_fdata* fd, struct sis1100_irq_ack* data)
{
        struct SIS1100_softc* sc=fd->sc;
        int irqs;

        irqs=fd->owned_irqs & data->irq_mask & sc->pending_irqs;

        _sis1100_irq_ack(sc, irqs);
        return 0;
}

int
sis1100_irq_get(struct SIS1100_fdata* fd, struct sis1100_irq_get* data)
{
        struct SIS1100_softc* sc=fd->sc;

        data->irqs=sc->pending_irqs & fd->owned_irqs;
        if (fd->old_remote_ok!=sc->remote_ok) {
                if (sc->remote_ok)
                        data->remote_status=1;
                else
                        data->remote_status=-1;
                fd->old_remote_ok=sc->remote_ok;
        } else
                data->remote_status=0;

        _sis1100_irq_get_vector(sc, data->irqs & data->irq_mask, data);

        if (data->immediate_ack)
                _sis1100_irq_ack(sc, data->irqs & data->irq_mask);
        return 0;
}

int
sis1100_irq_wait(struct SIS1100_fdata* fd, struct sis1100_irq_get* data)
{
        struct SIS1100_softc* sc=fd->sc;
        int irqs, res;
/*
        printk(KERN_INFO "SIS1100[%d] irq_wait: mask=0x%08x, ack=%d\n",
                        sc->unit, data->irq_mask, data->immediate_ack);
        printk(KERN_INFO "irq_wait before wait: pending_irqs=0x%08x\n",
                        sc->pending_irqs);
        printk(KERN_INFO "sc->ok=%d sc->old_ok=%d fd->old_ok=%d\n",
                        sc->remote_ok,
                        sc->old_remote_ok,
                        fd->old_remote_ok);
*/
        irqs=fd->owned_irqs & data->irq_mask;

        res=wait_event_interruptible(sc->irq_wait,
                        ((sc->pending_irqs & irqs) ||
                        (fd->old_remote_ok!=sc->remote_ok)));
        if (res) return -EINTR;

        data->irqs=sc->pending_irqs & fd->owned_irqs;
        if (fd->old_remote_ok!=sc->remote_ok) {
                if (sc->remote_ok)
                        data->remote_status=1;
                else
                        data->remote_status=-1;
                fd->old_remote_ok=sc->remote_ok;
        } else
                data->remote_status=0;

        _sis1100_irq_get_vector(sc, data->irqs & data->irq_mask, data);

        if (data->immediate_ack)
                _sis1100_irq_ack(sc, data->irqs & data->irq_mask);

        return 0;
}

/* $ZEL: sis3100rem_irq.c,v 1.1 2003/01/09 12:16:05 wuestner Exp $ */

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
sis3100_irq_acknowledge(struct SIS1100_softc* sc, int level)
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
	printk(KERN_ERR "SIS1100/3100: error in Iack level %d: 0x%x\n",
                level, error);
        vector=-1;
    } else {
        vector=sis1100readreg(sc, tc_dal)&0xff;
    }
    up(&sc->sem_hw);
    return vector;
}

void
sis3100rem_irq_handler(struct SIS1100_softc* sc)
{
    int i;

    sc->new_irqs=sc->doorbell&~sc->pending_irqs;
    sc->doorbell=0;
    sc->pending_irqs|=sc->new_irqs;

    down(&sc->sem_fdata_list);
    /* block IRQs in VME controller*/
    if (sc->new_irqs & SIS3100_VME_IRQS) {
        sis3100writeremreg(sc, vme_irq_sc, (sc->new_irqs&SIS3100_VME_IRQS)<<16, 1);
    }
    /* obtain irq vectors from VME */
    for (i=7; i>0; i--) {
        if (sc->new_irqs & (1<<i)) {
            sc->irq_vects[i].vector=sis3100_irq_acknowledge(sc, i);
            sc->irq_vects[i].valid=1;
            /*printk(KERN_INFO "vme_irq_handler: level %d vector=0x%08x\n",
                i, sc->irq_info[i].vector);*/
        }
    }
    /* block and clear FRONT-IRQs in VME controller*/
    if (sc->new_irqs & SIS3100_EXT_IRQS) {
        sis3100writeremreg(sc, in_latch_irq, (sc->new_irqs&SIS3100_EXT_IRQS)<<8, 1);
        sis3100writeremreg(sc, in_latch_irq, (sc->new_irqs&SIS3100_EXT_IRQS)<<16, 1);
    }

    sis3100writeremreg(sc, vme_irq_sc, 1<<15, 1);
    /*sis3100writereg(sc, in_latch_irq, 1<<15, 1);*/
    up(&sc->sem_fdata_list);
}

void
sis3100rem_enable_irqs(struct SIS1100_fdata* fd, u_int32_t mask)
{
    struct SIS1100_softc* sc=fd->sc;
    if (mask & SIS3100_VME_IRQS) {
        mask&=SIS3100_VME_IRQS;
        sis3100writeremreg(sc, vme_irq_sc, mask, 0);
    }
    /* enable VME-FRONT-IRQs and SIS3100_DSP_IRQ */
    if (mask & SIS3100_EXT_IRQS) {
        mask&=SIS3100_EXT_IRQS;
        sis3100writeremreg(sc, in_latch_irq, mask<<16, 1);
        sis3100writeremreg(sc, in_latch_irq, mask>>8, 0);
    }
}

void
sis3100rem_disable_irqs(struct SIS1100_fdata* fd, u_int32_t mask)
{
    struct SIS1100_softc* sc=fd->sc;
    if (mask & SIS3100_VME_IRQS) {
        mask&=SIS3100_VME_IRQS;
        mask<<=16;
        sis3100writeremreg(sc, vme_irq_sc, mask, 0);
    }
    if (mask & SIS3100_EXT_IRQS) {
        mask&=SIS3100_EXT_IRQS;
        mask<<=8;
        sis3100writeremreg(sc, in_latch_irq, mask, 0);
    }
}

void
sis3100rem_get_vector(struct SIS1100_softc* sc, int irqs,
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

void
sis3100rem_irq_ack(struct SIS1100_softc* sc, int irqs)
{
    if (irqs & SIS3100_VME_IRQS)
        sis3100writeremreg(sc, vme_irq_sc, irqs & SIS3100_VME_IRQS, 0);

    if (irqs & SIS3100_EXT_IRQS)
        sis3100writeremreg(sc, in_latch_irq, (irqs&SIS3100_EXT_IRQS)>>8, 0);
}

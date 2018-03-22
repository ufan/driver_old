/* $ZEL: sis1100_synch_handler.c,v 1.1 2003/01/09 12:16:05 wuestner Exp $ */

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

void
sis1100_synch_s_handler(unsigned long data)
{
    struct SIS1100_softc* sc=(struct SIS1100_softc*)data;
    unsigned long flags;

/*
printk(KERN_WARNING "synch_s_handler called\n");
*/

    spin_lock_irqsave(&sc->handlercommand.lock, flags);
    sc->handlercommand.command|=handlercomm_synch;
    spin_unlock_irqrestore(&sc->handlercommand.lock, flags);
    wake_up(&sc->handler_wait);
}

void
sis1100_synch_handler(struct SIS1100_softc* sc)
{
    u_int32_t status;

/*
printk(KERN_WARNING "synch_handler called\n");
*/

    sis1100_enable_irq(sc, 0, irq_synch_chg|irq_reset_req|irq_prot_l_err);
    status=sis1100readreg(sc, sr);

/*
    printk(KERN_INFO "SIS1100[%d]: synch_handler: status=0x%08x\n",
        sc->unit, status);
*/

    if ((status&sr_synch)==sr_synch) {
        if (sc->remote_hw==sis1100_hw_invalid)
            sis1100_init_remote(sc);
        else
            printk(KERN_INFO "synch_handler: remote_hw=%d\n", sc->remote_hw);
    }
}

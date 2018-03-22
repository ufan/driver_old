/* $ZEL: sis1100_irq.c,v 1.18 2003/01/09 12:13:17 wuestner Exp $ */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/wrapper.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>

#include <dev/pci/sis1100_sc.h>

int
sis1100_enable_irq(struct SIS1100_softc* sc,
    u_int32_t plx_mask, u_int32_t sis_mask)
{
    unsigned long flags;
    if (plx_mask) {
        spin_lock_irqsave(&sc->lock_intcsr, flags);
        plxwritereg(sc, INTCSR, plxreadreg(sc, INTCSR)|plx_mask);
        spin_unlock_irqrestore(&sc->lock_intcsr, flags);
    }

    if (sis_mask) {
        sis_mask&=sis1100_all_irq;
        sis1100writereg(sc, sr, sis_mask); /* clear pending irqs */
        sis1100writereg(sc, cr, sis_mask); /* enable irqs */
    }
    return 0;
}

int
sis1100_disable_irq(struct SIS1100_softc* sc,
    u_int32_t plx_mask, u_int32_t sis_mask)
{
    unsigned long flags;
    if (plx_mask) {
        spin_lock_irqsave(&sc->lock_intcsr, flags);
        plxwritereg(sc, INTCSR, plxreadreg(sc, INTCSR)&~plx_mask);
        spin_unlock_irqrestore(&sc->lock_intcsr, flags);
    }

    if (sis_mask) sis1100writereg(sc, cr, (sis_mask&sis1100_all_irq)<<16);
    return 0;
}

/*Doorbell | Local | DMA0 | DMA1 */
#define HANDLED_IRQS (0x2000|0x8000|0x200000|0x400000)

void
SIS1100_intr(int irq, void *vsc, struct pt_regs *regs)
{
    struct SIS1100_softc* sc=(struct SIS1100_softc*)vsc;
    unsigned long flags;
    u_int32_t intcsr;
    int local, handler_command, wakeup_remote, wakeup_local;

    intcsr=plxreadreg(sc, INTCSR);
    if (!(intcsr & HANDLED_IRQS)) return;

    local=0;
    handler_command=0;
    wakeup_remote=0;
    wakeup_local=0;

    if (intcsr&0x2000) { /* Doorbell Interrupt (== VME/CAMAC IRQ) */
        u_int32_t help=plxreadreg(sc, L2PDBELL);
        printk(KERN_WARNING "doorbell irq: doorbell=0x%x\n", help);
        sc->doorbell|=help;
        plxwritereg(sc, L2PDBELL, help);
        handler_command|=handlercomm_doorbell;
        wakeup_remote=1;
    }
    if (intcsr&0x8000) { /* local Interrupt */
        local=1;
    }
    if (intcsr&0x200000) { /* DMA0 Interrupt */
        spin_lock_irqsave(&sc->lock_intcsr, flags);
        plxwritereg(sc, INTCSR, intcsr&~(1<<18));
        spin_unlock_irqrestore(&sc->lock_intcsr, flags);
        sc->got_irqs|=got_dma0;
        wakeup_local=1;
    }
    if (intcsr&0x400000) { /* DMA1 Interrupt */
        spin_lock_irqsave(&sc->lock_intcsr, flags);
        plxwritereg(sc, INTCSR, intcsr&~(1<<19));
        spin_unlock_irqrestore(&sc->lock_intcsr, flags);
        sc->got_irqs|=got_dma1;
        wakeup_local=1;
    }
    if (local) {
        u_int32_t status;

        status=sis1100readreg(sc, sr);
        if (status&irq_synch_chg) {
            sis1100_disable_irq(sc, 0, irq_synch_chg|
                irq_reset_req|irq_prot_end|irq_prot_l_err);

            sc->got_irqs|=got_sync;
            wakeup_local=1;

            if ((status&3)==3) {
                printk(KERN_WARNING "----------------------\n");
                printk(KERN_WARNING "SIS1100[%d] %s: link is UP\n",
                        sc->unit, sc->pcidev->slot_name);
            } else {
                printk(KERN_WARNING "------------------------\n");
                printk(KERN_WARNING "SIS1100[%d] %s: link is DOWN\n",
                    sc->unit, sc->pcidev->slot_name);
                sc->old_remote_hw=sc->remote_hw;
                sc->remote_hw=sis1100_hw_invalid;
                handler_command|=handlercomm_synch;
                wakeup_remote=1;
            }
            printk(KERN_INFO "SIS1100[%d] status =0x%08x\n", sc->unit, status);
            mod_timer(&sc->link_up_timer, jiffies+HZ);
        }
        if (status&irq_inh_chg)
                        printk(KERN_ALERT "SIS1100[%d]: INH_CHG\n", sc->unit);
        if (status&irq_sema_chg)
                        printk(KERN_ALERT "SIS1100[%d]: SEMA_CHG\n", sc->unit);
        if (status&irq_rec_violation)
                        printk(KERN_ALERT "SIS1100[%d]: REC_VIOLATION\n", sc->unit);
        if (status&irq_reset_req)
                        printk(KERN_ALERT "SIS1100[%d]: RESET_REQ\n", sc->unit);
        if (status&irq_dma_eot) {
            sc->got_irqs|=got_eot;
            wakeup_local=1;
        }
        if (status&irq_mbx0) printk(KERN_INFO "SIS1100[%d]: MBX0\n", sc->unit);
        if (status&irq_s_xoff) {
            printk(KERN_INFO "SIS1100[%d]: S_XOFF\n", sc->unit);
            printk(KERN_INFO "SIS1100[%d]: status=0x%08x\n", sc->unit, status);
            sc->got_irqs|=got_xoff;
            wakeup_local=1;
        }
        if (status&irq_lemo_in_chg) {
                printk(KERN_INFO "SIS1100[%d]: LEMO_IN_CHG, status=0x%08x\n",
                        sc->unit, status);
            /*sc->doorbell|=(status<<4)&0x30000;*/
            handler_command|=handlercomm_lemo;
            wakeup_remote=1;
        }
        if (status&irq_prot_end) {
            sc->got_irqs|=got_end;
            wakeup_local=1;
        }
        if (status&irq_prot_l_err) {
            /*printk(KERN_INFO "SIS1100[%d]: PROT_L_ERR\n", sc->unit);*/
            sc->got_irqs|=got_l_err;
            wakeup_local=1;
        }
        sis1100writereg(sc, sr, status);
    }

    if (wakeup_local) wake_up_interruptible(&sc->local_wait);
    if (wakeup_remote) {
        spin_lock_irqsave(&sc->handlercommand.lock, flags);
        sc->handlercommand.command=handler_command;
        spin_unlock_irqrestore(&sc->handlercommand.lock, flags);
        wake_up(&sc->handler_wait);
    }
}

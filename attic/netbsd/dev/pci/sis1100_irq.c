/* $ZEL: sis1100_irq.c,v 1.4 2003/01/15 14:17:01 wuestner Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/sis1100_var.h>
#include <dev/pci/sis1100_sc.h>

int
sis1100_enable_irq(struct SIS1100_softc* sc,
    u_int32_t plx_mask, u_int32_t sis_mask)
{
    if (plx_mask) {
        int s;
        s=splbio();
        simple_lock(sc->lock_intcsr);
        plxwritereg(sc, INTCSR, plxreadreg(sc, INTCSR)|plx_mask);
        simple_unlock(sc->lock_intcsr);
        splx(s);
    }

    if (sis_mask) {
        sis_mask&=sis1100_all_irq;
/* clear pending irqs */
        sis1100writereg(sc, sr, sis_mask);
/* enable irqs */
        sis1100writereg(sc, cr, sis_mask);
    }
    return 0;
}

int
sis1100_disable_irq(struct SIS1100_softc* sc,
    u_int32_t plx_mask, u_int32_t sis_mask)
{
    if (plx_mask) {
        int s;
        s=splbio();
        simple_lock(sc->lock_intcsr);
        plxwritereg(sc, INTCSR, plxreadreg(sc, INTCSR)&~plx_mask);
        simple_unlock(sc->lock_intcsr);
        splx(s);
    }

    if (sis_mask) sis1100writereg(sc, cr, (sis_mask&sis1100_all_irq)<<16);
    return 0;
}

/*Doorbell | Local | DMA0 | DMA1 */
#define HANDLED_IRQS (0x2000|0x8000|0x200000|0x400000)

int
sis1100_intr(void* vsc)
{
    struct SIS1100_softc* sc=(struct SIS1100_softc*)vsc;
    u_int32_t intcsr;
    int local=0;
    int wakeup_vmeirq=0;
    int wakeup_local=0;

    intcsr=plxreadreg(sc, INTCSR);
    if (!(intcsr & HANDLED_IRQS)) return 0;

    if (intcsr&0x2000) { /* Doorbell Interrupt (== VME IRQ) */
        sc->doorbell|=plxreadreg(sc, L2PDBELL);
        /*plxwritereg(sc, L2PDBELL, sc->doorbell);*/
        plxwritereg(sc, L2PDBELL, 0xffffffff);
        wakeup_vmeirq=1;
    }
    if (intcsr&0x8000) { /* local Interrupt */
        local=1;
    }
    if (intcsr&0x200000) { /* DMA0 Interrupt */
        simple_lock(sc->lock_intcsr);
        plxwritereg(sc, INTCSR, intcsr&~(1<<18));
        simple_unlock(sc->lock_intcsr);
        sc->got_irqs|=got_dma0;
        wakeup_local=1;
    }
    if (intcsr&0x400000) { /* DMA1 Interrupt */
        simple_lock(sc->lock_intcsr);
        plxwritereg(sc, INTCSR, intcsr&~(1<<19));
        simple_unlock(sc->lock_intcsr);
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
                printf("----------------------\n");
                printf("%s: link is UP\n",
                        sc->sc_dev.dv_xname);
            } else {
                printf("------------------------\n");
                printf("%s: link is DOWN\n",
                    sc->sc_dev.dv_xname);
                sc->old_remote_hw=sc->remote_hw;
                simple_lock(&sc->vmeirq_wait);
                sc->remote_hw=sis1100_hw_invalid;
                simple_unlock(&sc->vmeirq_wait);
                wakeup_vmeirq=1;
            }
            printf("%s status =0x%08x\n", sc->sc_dev.dv_xname, status);
            printf("%s opt_csr=0x%08x\n",
                    sc->sc_dev.dv_xname, sis1100readreg(sc, opt_csr));
            callout_reset(&sc->link_up_timer, hz, sis1100_synch_handler, sc);
        }
        if (status&irq_inh_chg)
                        printf("%s: INH_CHG\n", sc->sc_dev.dv_xname);
        if (status&irq_sema_chg)
                        printf("%s: SEMA_CHG\n", sc->sc_dev.dv_xname);
        if (status&irq_rec_violation)
                        printf("%s: REC_VIOLATION\n", sc->sc_dev.dv_xname);
        if (status&irq_reset_req)
                        printf("%s: RESET_REQ\n", sc->sc_dev.dv_xname);
        if (status&irq_dma_eot) {
            sc->got_irqs|=got_eot;
            wakeup_local=1;
        }
        if (status&irq_mbx0) printf("%s: MBX0\n", sc->sc_dev.dv_xname);
        if (status&irq_s_xoff) {
            printf("%s: S_XOFF\n", sc->sc_dev.dv_xname);
            printf("%s: status=0x%08x\n", sc->sc_dev.dv_xname, status);
            sc->got_irqs|=got_xoff;
            wakeup_local=1;
        }
        if (status&irq_lemo_in_chg) {
                printf("%s: LEMO_IN_CHG, status=0x%08x\n",
                        sc->sc_dev.dv_xname, status);
            sc->doorbell|=(status<<4)&0x30000;
            wakeup_vmeirq=1;
        }
        if (status&irq_prot_end) {
            sc->got_irqs|=got_end;
            wakeup_local=1;
        }
        if (status&irq_prot_l_err) {
            /*printf("%s: PROT_L_ERR\n", sc->sc_dev.dv_xname);*/
            sc->got_irqs|=got_l_err;
            wakeup_local=1;
        }
        sis1100writereg(sc, sr, status);
    }

    if (wakeup_local) wakeup(&sc->local_wait);
    if (wakeup_vmeirq) wakeup(&sc->vmeirq_wait);

    return 1;
}

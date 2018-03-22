/* $ZEL: sis1100_init.c,v 1.19 2003/01/09 12:13:16 wuestner Exp $ */

#include "Copyright"

#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>

#include <dev/pci/sis1100_sc.h>

void
sis1100_dump_glink_status(struct SIS1100_softc* sc, char* text, int locked)
{
    u_int32_t v;
    if (!locked) down(&sc->sem_hw);
    pr_info("%s:\n", text);
    pr_info("  ident       =%08x\n", sis1100readreg(sc, ident));
    pr_info("  sr          =%08x\n", sis1100readreg(sc, sr));
    pr_info("  cr          =%08x\n", sis1100readreg(sc, cr));
    pr_info("  t_hdr       =%08x\n", sis1100readreg(sc, t_hdr));
    pr_info("  t_am        =%08x\n", sis1100readreg(sc, t_am));
    pr_info("  t_adl       =%08x\n", sis1100readreg(sc, t_adl));
    pr_info("  t_dal       =%08x\n", sis1100readreg(sc, t_dal));
    pr_info("  tc_hdr      =%08x\n", sis1100readreg(sc, tc_hdr));
    pr_info("  tc_dal      =%08x\n", sis1100readreg(sc, tc_dal));
    pr_info("  p_balance   =%08x\n", sis1100readreg(sc, p_balance));
    pr_info("  prot_error  =%08x\n", sis1100readreg(sc, prot_error));
    pr_info("  d0_bc       =%08x\n", sis1100readreg(sc, d0_bc));
    pr_info("  d0_bc_buf   =%08x\n", sis1100readreg(sc, d0_bc_buf));
    pr_info("  d0_bc_blen  =%08x\n", sis1100readreg(sc, d0_bc_blen));
    pr_info("  d_hdr       =%08x\n", sis1100readreg(sc, d_hdr));
    pr_info("  d_am        =%08x\n", sis1100readreg(sc, d_am));
    pr_info("  d_adl       =%08x\n", sis1100readreg(sc, d_adl));
    pr_info("  d_bc        =%08x\n", sis1100readreg(sc, d_bc));
    pr_info("  rd_pipe_buf =%08x\n", sis1100readreg(sc, rd_pipe_buf));
    pr_info("  rd_pipe_blen=%08x\n", sis1100readreg(sc, rd_pipe_blen));
    pr_info("\n");
    v=sis1100readreg(sc, opt_csr);
    pr_info("  opt_csr     =%08x\n", v);
    sis1100writereg(sc, opt_csr, v&0xc0f50000);
    pr_info("  opt_csr     =%08x\n", sis1100readreg(sc, opt_csr));
    if (!locked) up(&sc->sem_hw);
}

void
sis1100_flush_fifo(struct SIS1100_softc* sc, const char* text, int silent)
{
    u_int32_t sr, special, data;
    int count=0;

    sis1100writereg(sc, cr, cr_transparent);
    mb();
    sr=sis1100readreg(sc, sr);
    while (sr&(sr_tp_special|sr_tp_data)) {
        while (sr&sr_tp_data) {
            data=sis1100readreg(sc, tp_data);
            if (!silent) pr_info("data   =          0x%08x\n", data);
            sr=sis1100readreg(sc, sr);
        }
        while ((sr&(sr_tp_special|sr_tp_data))==sr_tp_special) {
            special=sis1100readreg(sc, tp_special);
            if (!silent) pr_info("special=0x%08x\n", special);
            sr=sis1100readreg(sc, sr);
        }
        count++;
    }
    sis1100writereg(sc, cr, cr_transparent<<16);
    if (count && silent)
        pr_info("SIS1100[%d]: flushed %d words from fifo\n", sc->unit, count);
}

int
SIS1100_init(struct SIS1100_softc* sc)
{
#define MIN_FV 5
#define MAX_FV 7

    u_int32_t typ, hv, fk, fv;
    int res, i;

    sc->local_ident=sis1100readreg(sc, ident);
    typ=sc->local_ident&0xff;
    hv=(sc->local_ident>>8)&0xff;
    fk=(sc->local_ident>>16)&0xff;
    fv=(sc->local_ident>>24)&0xff;
    if (typ!=1) {
    	printk(KERN_ERR "SIS1100[%d]: ident=08x%x; "
            "claims not to be a PCI Device\n", sc->unit, typ);
    	res=-ENXIO;
    	goto raus;
    }
    printk(KERN_INFO "SIS1100[%d]: HW version %d; FW code %d; FW version %d\n",
        sc->unit, hv, fk, fv);

    switch (sc->local_ident&0xffff00) { /* HW version and FW code */
    	case 0x010100: {
    	    if (fv<MIN_FV) {
                printk(KERN_ERR "SIS1100[%d]: Firmware version too old;"
                        " at least version %d is required.\n",
                        sc->unit, MIN_FV);
                res=-ENXIO;
    	    	goto raus;
            }
            if (fv>MAX_FV)
                printk(KERN_WARNING "SIS1100[%d]: Driver not tested with"
                        " firmware versions greater than %d.\n",
                        sc->unit, MAX_FV);
    	    if (sc->reg_size!=0x1000) {
    	    	printk(KERN_ERR "SIS1100[%d]: wrong size of space 0: "
    	    	    "0x%x instead of 0x1000\n"
                    , sc->unit, sc->reg_size);
    	    	res=-ENXIO;
    	    	goto raus;
    	    }
	    printk(KERN_INFO "SIS1100[%d]: size of space 1: "
		"0x%x (%d MByte)\n",
		sc->unit, sc->rem_size, sc->rem_size>>20);
        } break;
    	default:
    	    printk(KERN_ERR "SIS1100[%d]: Hard- or Firmware not known\n",
                        sc->unit);
    	    res=-ENXIO;
    	    goto raus;
    }

    /* reset all we can */
    sis1100writereg(sc, cr, cr_reset); /* master reset */
    sis1100writereg(sc, cr, cr_rem_reset); /* reset remote, ignore wether it exists */
    sis1100_flush_fifo(sc, "init", 0); /* clear local fifo */
    sis1100writereg(sc, cr, cr_reset); /* master reset again */
    sis1100_reset_plx(sc);             /* reset PLX */
    sis1100writereg(sc, p_balance, 0);
    sis1100readreg(sc, prot_error);

    /* sis1100_dump_glink_status(sc, "INITIAL DUMP"); */

    /* enable PCI Initiator-to-PCI Memory */
    plxwritereg(sc, DMRR, 0);
    plxwritereg(sc, DMLBAM, 0);
    plxwritereg(sc, DMPBAM, 1);

    sis1100writereg(sc, cr, 8); /* big endian */

    sc->got_irqs=0;
    for (i=0; i<=7; i++) sc->irq_vects[i].valid=0;
    sc->pending_irqs=0;
    sc->doorbell=0;

    /* enable IRQs */
    sis1100_disable_irq(sc, 0xffffffff, 0xffffffff);
    sis1100_enable_irq(sc, plxirq_pci|plxirq_mbox|plxirq_doorbell|plxirq_local,
	    irq_synch_chg|irq_inh_chg|irq_sema_chg|
	    irq_rec_violation|irq_reset_req|irq_mbx0);

    sc->dsp_present=0;
    sc->ram_size=0;
    sc->remote_ident=0;
    sc->old_remote_hw=sis1100_hw_invalid;
    sc->remote_hw=sis1100_hw_invalid;

    if ((sis1100readreg(sc, sr)&sr_synch)==sr_synch) {
    	sis1100_init_remote(sc);
    } else {
    	printk(KERN_WARNING "SIS1100[%d] init: remote interface not reachable\n",
                sc->unit);
    }
    res=0;

    raus:
    return res;
#undef MIN_FV
#undef MAX_FV
}

void
SIS1100_done(struct SIS1100_softc* sc)
{
    /* DMA Ch. 0/1: not enabled */
    plxwritereg(sc, DMACSR0_DMACSR1, 0);
    /* disable interrupts */
    plxwritereg(sc, INTCSR, 0);
}

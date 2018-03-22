/* $ZEL: sis1100_init.c,v 1.3 2003/01/15 14:16:59 wuestner Exp $ */

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

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/pci/sis1100_var.h>
#include <dev/pci/sis1100_sc.h>

void
dump_glink_status(struct SIS1100_softc* sc, char* text, int locked)
{
    u_int32_t v;
    if (!locked) lockmgr(&sc->sem_hw, LK_EXCLUSIVE, 0);
    printf("%s:\n", text);
    printf("  ident       =%08x\n", sis1100readreg(sc, ident));
    printf("  sr          =%08x\n", sis1100readreg(sc, sr));
    printf("  cr          =%08x\n", sis1100readreg(sc, cr));
    printf("  t_hdr       =%08x\n", sis1100readreg(sc, t_hdr));
    printf("  t_am        =%08x\n", sis1100readreg(sc, t_am));
    printf("  t_adl       =%08x\n", sis1100readreg(sc, t_adl));
    printf("  t_dal       =%08x\n", sis1100readreg(sc, t_dal));
    printf("  tc_hdr      =%08x\n", sis1100readreg(sc, tc_hdr));
    printf("  tc_dal      =%08x\n", sis1100readreg(sc, tc_dal));
    printf("  p_balance   =%08x\n", sis1100readreg(sc, p_balance));
    printf("  prot_error  =%08x\n", sis1100readreg(sc, prot_error));
    printf("  d0_bc       =%08x\n", sis1100readreg(sc, d0_bc));
    printf("  d0_bc_buf   =%08x\n", sis1100readreg(sc, d0_bc_buf));
    printf("  d0_bc_blen  =%08x\n", sis1100readreg(sc, d0_bc_blen));
    printf("  d_hdr       =%08x\n", sis1100readreg(sc, d_hdr));
    printf("  d_am        =%08x\n", sis1100readreg(sc, d_am));
    printf("  d_adl       =%08x\n", sis1100readreg(sc, d_adl));
    printf("  d_bc        =%08x\n", sis1100readreg(sc, d_bc));
    printf("  rd_pipe_buf =%08x\n", sis1100readreg(sc, rd_pipe_buf));
    printf("  rd_pipe_blen=%08x\n", sis1100readreg(sc, rd_pipe_blen));
    printf("\n");
    v=sis1100readreg(sc, opt_csr);
    printf("  opt_csr     =%08x\n", v);
    sis1100writereg(sc, opt_csr, v&0xc0f50000);
    printf("  opt_csr     =%08x\n", sis1100readreg(sc, opt_csr));
    if (!locked) lockmgr(&sc->sem_hw, LK_RELEASE, 0);
}

void
flush_fifo(struct SIS1100_softc* sc, const char* text, int silent)
{
    u_int32_t sr, special, data;
    int count=0;

    sis1100writereg(sc, cr, cr_transparent);
    sr=sis1100readreg(sc, sr);
    while (sr&(sr_tp_special|sr_tp_data)) {
        while (sr&sr_tp_data) {
            data=sis1100readreg(sc, tp_data);
            if (!silent) printf("data   =          0x%08x\n", data);
            sr=sis1100readreg(sc, sr);
        }
        while ((sr&(sr_tp_special|sr_tp_data))==sr_tp_special) {
            special=sis1100readreg(sc, tp_special);
            if (!silent) printf("special=0x%08x\n", special);
            sr=sis1100readreg(sc, sr);
        }
        count++;
    }
    sis1100writereg(sc, cr, cr_transparent<<16);
    if (count && silent)
        printf("%s: flushed %d words from fifo\n", sc->sc_dev.dv_xname, count);
}

int
sis1100_init(sc)
	struct SIS1100_softc *sc;
{
#define MIN_FV 5
#define MAX_FV 7

    u_int32_t typ, hv, fk, fv;
    int res, i;

    sc->sc_inuse = 0;
    sc->local_ident=sis1100readreg(sc, ident);
    typ=sc->local_ident&0xff;
    hv=(sc->local_ident>>8)&0xff;
    fk=(sc->local_ident>>16)&0xff;
    fv=(sc->local_ident>>24)&0xff;
    if (typ!=1) {
    	printf("%s: ident=08x%x; "
            "claims not to be an PCI Device\n",
            sc->sc_dev.dv_xname, sc->local_ident);
    	res=ENXIO;
    	goto raus;
    }
    printf("%s: HW version %d; FW code %d; FW version %d\n",
        sc->sc_dev.dv_xname, hv, fk, fv);

    switch (sc->local_ident&0xffff00) { /* HW version and FW code */
    	case 0x010100: {
    	    if (fv<MIN_FV) {
                printf("%s: Firmware version too old;"
                        " at least version %d is required.\n",
                        sc->sc_dev.dv_xname, MIN_FV);
                res=ENXIO;
    	    	goto raus;
            }
            if (fv>MAX_FV)
                printf("%s: Driver not tested with"
                        " firmware versions greater than %d.\n",
                        sc->sc_dev.dv_xname, MAX_FV);
    	    if (sc->reg_size!=0x1000) {
    	    	printf("%s: wrong size of space 0: "
    	    	    "0x%lx instead of 0x1000\n",
                            sc->sc_dev.dv_xname, sc->reg_size);
    	    	res=ENXIO;
    	    	goto raus;
    	    }
	    printf("%s: size of space 1: "
		"0x%lx (%ld MByte)\n",
		sc->sc_dev.dv_xname, sc->rem_size, sc->rem_size>>20);
        } break;
    	default:
    	    printf("%s: Hard- or Firmware not known\n",
                        sc->sc_dev.dv_xname);
    	    res=ENXIO;
    	    goto raus;
    }

    /* reset all we can */
    sis1100writereg(sc, cr, cr_reset); /* master reset */
    sis1100writereg(sc, cr, cr_rem_reset); /* reset remote, ignore wether it exists */
    flush_fifo(sc, "init", 0); /* clear local fifo */
    sis1100writereg(sc, cr, cr_reset); /* master reset again */
    sis1100_reset_plx(sc);             /* reset PLX */
    sis1100writereg(sc, p_balance, 0);
    sis1100readreg(sc, prot_error);

    /* dump_glink_status(sc, "INITIAL DUMP"); */

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
    sc->dsp_size=0;
    sc->ram_size=0;
    sc->remote_ident=0;
    sc->old_remote_hw=sis1100_hw_invalid;
    sc->remote_hw=sis1100_hw_invalid;

    if ((sis1100readreg(sc, sr)&sr_synch)==sr_synch) {
    	sis1100_init_remote(sc);
    } else {
    	printf("%s init: remote interface not reachable\n",
                sc->sc_dev.dv_xname);
    }

    res=0;

    raus:
    return res;
}

void
sis1100_done(struct SIS1100_softc* sc)
{
    /* DMA Ch. 0/1: not enabled */
    plxwritereg(sc, DMACSR0_DMACSR1, 0);
    /* disable interrupts */
    plxwritereg(sc, INTCSR, 0);
}

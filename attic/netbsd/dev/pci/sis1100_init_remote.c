/* $ZEL: sis1100_init_remote.c,v 1.6 2003/01/15 14:17:00 wuestner Exp $ */

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

#include <dev/pci/sis1100_sc.h>

void
sis1100_synch_handler(void* data)
{
    struct SIS1100_softc* sc=(struct SIS1100_softc*)data;
    printf("sis1100_synch_handler called\n");
    wakeup(&sc->sync_pp);
}

void
sis1100thread_sync(void* data)
{
    struct SIS1100_softc* sc=(struct SIS1100_softc*)data;
    u_int32_t status;
    int command;

    printf("sis1100thread_sync started\n");
    while (1) {
        tsleep(&sc->sync_pp, PCATCH, "sis1100thread_sync", 0);
        command=sc->sync_command.command;
        if (command<0) {
            printf("sis1100thread_sync terminated\n");
            simple_lock(&sc->sync_command.lock);
            sc->sync_command.command=0;
            simple_unlock(&sc->sync_command.lock);
            wakeup(sc);
            kthread_exit(0);
        }
        printf("sis1100thread_sync aufgewacht\n");
/* reenable IRQs, but not sis1100irq_prot_end */
        sis1100_enable_irq(sc, 0, irq_synch_chg|irq_reset_req|irq_prot_l_err);
        status=sis1100readreg(sc, sr);

        printf("%s: sis1100thread_sync: status=0x%08x\n",
            sc->sc_dev.dv_xname, status);

        if ((status&sr_synch)==sr_synch) sis1100_init_remote(sc);
    }
}

char* rnames[]={"PCI", "VME", "CAMAC"};

void
sis1100_init_remote(struct SIS1100_softc* sc)
{
    u_int32_t ident, error, balance, typ, hv, fk, fv;
    int s, res;

    lockmgr(&sc->sem_hw, LK_EXCLUSIVE, 0);

    /*sis1100writereg(sc, cr, cr_rem_reset);*/ /* reset remote */
    flush_fifo(sc, "init_remote" , 0); /* clear local fifo */
    sis1100writereg(sc, p_balance, 0);
    sis1100readreg(sc, prot_error);

    ident=plxreadlocal0(sc, 0x800);
    error=sis1100readreg(sc, prot_error);
    balance=sis1100readreg(sc, p_balance);
    lockmgr(&sc->sem_hw, LK_RELEASE, 0);

    if (error || balance) {
        printf("%s: error reading remote ident\n", sc->sc_dev.dv_xname);
        printf("error=0x%x balance=%d\n", error, balance);
        flush_fifo(sc, "after reading ident" , 0); /* clear local fifo */
        /*dump_glink_status(sc, "init remote");*/
        return;
    }

    typ=ident&0xff;
    hv=(ident>>8)&0xff;
    fk=(ident>>16)&0xff;
    fv=(ident>>24)&0xff;
    printf("%s: remote ident: 0x%08x\n", sc->sc_dev.dv_xname, ident);
    if ((typ>0) && (typ<4))
        printf("%s: remote is %s\n", sc->sc_dev.dv_xname, rnames[typ-1]);
    else
        printf("%s: unknown remote type %d\n",
                sc->sc_dev.dv_xname, ident&0xff);
    printf("%s: remote HW_ver %d FW_code %d FW_ver %d\n",
                sc->sc_dev.dv_xname, hv, fk, fv);
    sc->remote_ident=ident;

    switch (typ) {
        case 1: /* PCI */
            res=sis1100_remote_init(sc);
            break;
        case 2: /* VME */
            res=sis3100_remote_init(sc);
            break;
        case 3: /* CAMAC */
            res=sis5100_remote_init(sc);
            break;
        default:
            printf("%s: remote device type not (yet?) supported.\n",
                    sc->sc_dev.dv_xname);
            res=-1;
            break;
    }
    if (res) return;

    sc->old_remote_hw=sc->remote_hw;
    s=splbio();
    simple_lock(&sc->vmeirq_wait);
    sc->remote_hw=typ;
    simple_unlock(&sc->vmeirq_wait);
    splx(s);
    wakeup(&sc->vmeirq_wait);
}

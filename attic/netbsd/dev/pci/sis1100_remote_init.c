/* $ZEL: sis1100_remote_init.c,v 1.1 2003/01/15 15:20:06 wuestner Exp $ */

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

int
sis1100_remote_init(struct SIS1100_softc* sc)
{
#define MIN_FV 4
#define MAX_FV 7
    u_int32_t hv, fk, fv;

    hv=(sc->remote_ident>>8)&0xff;
    fk=(sc->remote_ident>>16)&0xff;
    fv=(sc->remote_ident>>24)&0xff;

    switch (sc->remote_ident&0x00ffff00) {
    case 0x00010100:
        if (fv<MIN_FV) {
            printf("%s: remote firmware version too old;"
                    " at least version %d is required.\n",
                    sc->sc_dev.dv_xname, MIN_FV);
            return -1;
        }
        if (fv>MAX_FV) {
            printf("%s: Driver not tested with"
                    " remote firmware versions greater than %d.\n",
                    sc->sc_dev.dv_xname, MAX_FV);
        }
        break;
    default:
        printf("%s: remote hw/fw type not supported\n", sc->sc_dev.dv_xname);
        return -1;
    }

    sc->ram_size=0;

    sc->dsp_present=0;
    sc->dsp_size=0;

#if 0
    lockmgr(&sc->sem_hw, LK_EXCLUSIVE, 0);
    sis3100writereg(sc, vme_master_sc, 3<<14, 1);
    sis3100writereg(sc, vme_irq_sc, 0x00fe0001, 1);
    sis3100readreg(sc, vme_master_sc, &stat, 1);
    lockmgr(&sc->sem_hw, LK_RELEASE, 0);
    printf("%s: remote stat=0x%08x\n", sc->sc_dev.dv_xname, stat);
    if (!(stat&0x10000)) {
        printf("%s: System Controller NOT enabled!\n", sc->sc_dev.dv_xname);
    }
#endif

    return 0;
}

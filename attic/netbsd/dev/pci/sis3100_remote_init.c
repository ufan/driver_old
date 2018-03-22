/* $ZEL: sis3100_remote_init.c,v 1.1 2003/01/15 15:20:07 wuestner Exp $ */

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

static int
sis3100_dsp_present(struct SIS1100_softc* sc)
{
    u_int32_t dsp_sc;
    int res;

    res=sis3100readremreg(sc, dsp_sc, &dsp_sc, 0);
    if (res) {
        printf("%s: read dsp_sc: res=%d\n", sc->sc_dev.dv_xname, res);
        return 0;
    }
    return !!(dsp_sc&sis3100_dsp_available);
}

int
sis3100_remote_init(struct SIS1100_softc* sc)
{
#define MIN_FV 3
#define MAX_FV 5
    u_int32_t hv, fk, fv, stat;

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

    if (init_sdram(sc)<0)
        return -1;
    printf("%s: size of SDRAM: 0x%llx (%lld MByte)\n",
        sc->sc_dev.dv_xname, sc->ram_size, sc->ram_size>>20);

    sc->dsp_present=sis3100_dsp_present(sc);
    sc->dsp_size=sc->dsp_present?0x400000:0;
    printf("%s: SHARC is %spresent\n",
        sc->sc_dev.dv_xname, sc->dsp_present?"":"not ");

    lockmgr(&sc->sem_hw, LK_EXCLUSIVE, 0);
    sis3100writeremreg(sc, vme_master_sc, 3<<14, 1);
    sis3100writeremreg(sc, vme_irq_sc, 0x00fe0001, 1);
    sis3100readremreg(sc, vme_master_sc, &stat, 1);
    lockmgr(&sc->sem_hw, LK_RELEASE, 0);
    printf("%s: remote stat=0x%08x\n", sc->sc_dev.dv_xname, stat);
    if (!(stat&0x10000)) {
        printf("%s: System Controller NOT enabled!\n", sc->sc_dev.dv_xname);
    }
    {
    int berr_timer=(stat>>14)&3;
    int long_timer=(stat>>12)&3;
    int berr_time=0, long_time=0;
    switch (berr_timer) {
        case 0: berr_time=1250; break;
        case 1: berr_time=6250; break;
        case 2: berr_time=12500; break;
        case 3: berr_time=100000; break;
    }
    switch (long_timer) {
        case 0: long_time=1; break;
        case 1: long_time=10; break;
        case 2: long_time=50; break;
        case 3: long_time=100; break;
    }
    printf("%s: berr_time=%d ns\n", sc->sc_dev.dv_xname, berr_time);
    printf("%s: long_time=%d ms\n", sc->sc_dev.dv_xname, long_time);
    }

    return 0;
}

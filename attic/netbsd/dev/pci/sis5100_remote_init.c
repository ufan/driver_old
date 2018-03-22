/* $ZEL: sis5100_remote_init.c,v 1.1 2003/01/15 15:20:07 wuestner Exp $ */

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
sis5100_dsp_present(struct SIS1100_softc* sc)
{
    u_int32_t dsp_sc;
    int res;

    res=sis5100readremreg(sc, dsp_sc, &dsp_sc, 0);
    if (res) {
        printf("%s/5100: read dsp_sc: res=%d\n", sc->sc_dev.dv_xname, res);
        return 0;
    }
    return !!(dsp_sc&sis5100_dsp_available);
}

int
sis5100_remote_init(struct SIS1100_softc* sc)
{
#define MIN_FV 0
#define MAX_FV 0
    u_int32_t hv, fk, fv;

    hv=(sc->remote_ident>>8)&0xff;
    fk=(sc->remote_ident>>16)&0xff;
    fv=(sc->remote_ident>>24)&0xff;

    switch (sc->remote_ident&0x00ffff00) {
    case 0x00010100:
        if (fv<MIN_FV) {
            printf("%s/5100: remote firmware version too old;"
                    " at least version %d is required.\n",
                    sc->sc_dev.dv_xname, MIN_FV);
            return -1;
        }
        if (fv>MAX_FV) {
            printf("%s/5100: Driver not tested with"
                    " remote firmware versions greater than %d.\n",
                    sc->sc_dev.dv_xname, MAX_FV);
        }
        break;
    default:
        printf("%s/5100: remote hw/fw type not supported\n",
                sc->sc_dev.dv_xname);
        return -1;
    }

    if (init_sdram(sc)<0)
        return -1;
    printf("%s/5100: size of SDRAM: 0x%llx (%lld MByte)\n",
        sc->sc_dev.dv_xname, sc->ram_size, sc->ram_size>>20);

    sc->dsp_present=sis5100_dsp_present(sc);
    sc->dsp_size=sc->dsp_present?0x400000:0;
    printf("%s/5100: SHARC is %spresent\n",
        sc->sc_dev.dv_xname, sc->dsp_present?"":"not ");

    return 0;
}

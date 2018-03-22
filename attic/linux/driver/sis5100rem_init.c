/* $ZEL: sis5100rem_init.c,v 1.1 2003/01/09 12:16:06 wuestner Exp $ */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/pci.h>

#include <dev/pci/sis1100_sc.h>

static int
sis5100_dsp_present(struct SIS1100_softc* sc)
{
    u_int32_t dsp_sc;
    int res;

    res=sis5100readremreg(sc, dsp_sc, &dsp_sc, 0);
    if (res) {
        printk(KERN_INFO "SIS1100/5100[%d]: read dsp_sc: res=%d\n", sc->unit, res);
        return 0;
    }
    return !!(dsp_sc&sis5100_dsp_available);
}

int
sis5100rem_init(struct SIS1100_softc* sc)
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
            printk(KERN_ERR "SIS1100/5100[%d]: remote firmware version too old;"
                    " at least version %d is required.\n",
                    sc->unit, MIN_FV);
            return -1;
        }
        if (fv>MAX_FV) {
            printk(KERN_WARNING "SIS1100/5100[%d]: Driver not tested with"
                    " remote firmware versions greater than %d.\n",
                    sc->unit, MAX_FV);
        }
        break;
    default:
        printk(KERN_ERR "SIS1100/5100[%d]: remote hw/fw type not supported\n",
                sc->unit);
        return -1;
    }

    if (sis3100_init_sdram(sc)<0)
        return -1;
    printk(KERN_INFO "SIS1100/5100[%d]: size of SDRAM: 0x%llx (%lld MByte)\n",
        sc->unit, sc->ram_size, sc->ram_size>>20);

    sc->dsp_present=sis5100_dsp_present(sc);
    printk(KERN_INFO "SIS1100/5100[%d]: DSP is %spresent\n",
        sc->unit, sc->dsp_present?"":"not ");

    return 0;
}

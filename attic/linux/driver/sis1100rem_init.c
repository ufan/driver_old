/* $ZEL: sis1100rem_init.c,v 1.1 2003/01/09 12:16:05 wuestner Exp $ */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/pci.h>

#include <dev/pci/sis1100_sc.h>

int
sis1100rem_init(struct SIS1100_softc* sc)
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
            printk(KERN_ERR "SIS1100/1100[%d]: remote firmware version too old;"
                    " at least version %d is required.\n",
                    sc->unit, MIN_FV);
            return -1;
        }
        if (fv>MAX_FV) {
            printk(KERN_WARNING "SIS1100/1100[%d]: Driver not tested with"
                    " remote firmware versions greater than %d.\n",
                    sc->unit, MAX_FV);
        }
        break;
    default:
        printk(KERN_INFO "SIS1100/1100[%d]: remote hw/fw type not supported\n",
                sc->unit);
        return -1;
    }

    sc->ram_size=0;
    sc->dsp_present=0;
    return 0;
}

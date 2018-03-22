/* $ZEL: sis1100_reset.c,v 1.5 2003/01/09 12:13:19 wuestner Exp $ */

#include "Copyright"

#include <linux/config.h>
#include <linux/module.h>
#include <linux/iobuf.h>
#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/slab.h>

#include <dev/pci/sis1100_sc.h>

/* only used in initialisation; no need for semaphores */

void sis1100_reset_plx(struct SIS1100_softc* sc)
{
    u_int32_t conf_3C;
    int c;

    pci_read_config_dword(sc->pcidev, 0x3c, &conf_3C);

    _plxwritebreg(sc, 0x6f, 0x40);
    wmb();
    _plxwritebreg(sc, 0x6f, 0x00);
    wmb();
    _plxwritebreg(sc, 0x6f, 0x20);
    mb();
    c=0;
    while ((!(plxreadreg(sc, LAS0RR)) || (c<10)) && (++c<50));
    _plxwritebreg(sc, 0x6f, 0x00);

    pci_write_config_dword(sc->pcidev, 0x3c, conf_3C);
}

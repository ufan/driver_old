/* $ZEL: sis1100_reset.c,v 1.1 2002/06/06 19:41:30 wuestner Exp $ */

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

/* only used in initialisation; no need for semaphores */

void sis1100_reset_plx(struct SIS1100_softc* sc)
{
    u_int32_t conf_3C;
    int c;
return;
    pci_conf_read(sc->sc_pc, sc->sc_pcitag, PCI_INTERRUPT_REG);

    bus_space_write_1(sc->plx_t, sc->plx_h, 0x6f, 0x40);
    bus_space_write_1(sc->plx_t, sc->plx_h, 0x6f, 0x00);
    bus_space_write_1(sc->plx_t, sc->plx_h, 0x6f, 0x20);
    c=0;
    while ((!(plxreadreg(sc, LAS0RR)) || (c<10)) && (++c<50));
    bus_space_write_1(sc->plx_t, sc->plx_h, 0x6f, 0x00);

    pci_conf_write(sc->sc_pc, sc->sc_pcitag, PCI_INTERRUPT_REG, conf_3C);
}

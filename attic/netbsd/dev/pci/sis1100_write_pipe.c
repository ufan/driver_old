/* $ZEL: sis1100_write_pipe.c,v 1.3 2002/07/04 15:46:13 wuestner Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/sis1100_var.h>
#include <dev/pci/sis1100_sc.h>

int
sis1100_writepipe(struct SIS1100_softc* sc, int32_t am,
    int space, int num, u_int32_t* data)
{
    u_int32_t error;
    u_int32_t head;
    int i;

    head=0x0f000404|(space&0x3f)<<16;

    lockmgr(&sc->sem_hw, LK_EXCLUSIVE, 0);
    if (am>=0) {
        head|=0x800;
        sis1100writereg(sc, t_am, am);
    }
    sis1100writereg(sc, t_hdr, head);
    for (i=0; i<num; i++) {
        sis1100writereg(sc, t_adl, *data++);
        sis1100writereg(sc, t_dal, *data++);
    }

    do {
        error=sis1100readreg(sc, prot_error);
    } while (error==0x005);
    lockmgr(&sc->sem_hw, LK_RELEASE, 0);

    return error;
}

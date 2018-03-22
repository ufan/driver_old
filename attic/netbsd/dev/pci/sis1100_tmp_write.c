/* $ZEL: sis1100_tmp_write.c,v 1.1 2002/06/06 19:41:30 wuestner Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/sis1100_var.h>
#include <dev/pci/sis1100_sc.h>

int
sis1100_tmp_write(struct SIS1100_softc* sc,
    	u_int32_t addr, int32_t am, u_int32_t size, int space, u_int32_t data)
{
    u_int32_t be;
    u_int32_t error;
    u_int32_t head;

    data=(data&(0xffffffffU>>((4-size)<<3)))<<((addr&3)<<3);

    be=((0x00f00000<<size)&0x0f000000)<<(addr&3);

    head=0x00000402|(space&0x3f)<<16|be;

    lockmgr(&sc->sem_hw, LK_EXCLUSIVE, 0);
    if (am>=0) {
        head|=0x800;
        sis1100writereg(sc, t_am, am);
    }
    sis1100writereg(sc, t_hdr, head);
    sis1100writereg(sc, t_dal, data);
    sis1100writereg(sc, t_adl, addr);
    do {
        error=sis1100readreg(sc, prot_error);
    } while (error==0x005);
    lockmgr(&sc->sem_hw, LK_RELEASE, 0);

    return error;
}

int
sis1100_remote_reg_write(struct SIS1100_softc* sc, u_int32_t offs,
    u_int32_t data, int locked)
{
    u_int32_t error;

    if (!locked) lockmgr(&sc->sem_hw, LK_EXCLUSIVE, 0);
    plxwritelocal0(sc, offs+0x800, data);
    error=sis1100readreg(sc, prot_error);
    if (!locked) lockmgr(&sc->sem_hw, LK_RELEASE, 0);
    return error;
}

/* $ZEL: sis1100_tmp_write.c,v 1.8 2002/08/12 10:36:40 wuestner Exp $ */

#include "Copyright"

#include <linux/config.h>
#include <linux/module.h>
#include <linux/pci.h>

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

    down(&sc->sem_hw);
    if (am>=0) {
        head|=0x800;
        sis1100writereg(sc, t_am, am);
    }
    sis1100writereg(sc, t_hdr, head);
    wmb();
    sis1100writereg(sc, t_dal, data);
    wmb();
    sis1100writereg(sc, t_adl, addr);
    mb();
    do {
        error=sis1100readreg(sc, prot_error);
    } while (error==0x005);
    up(&sc->sem_hw);

    return error;
}

int
sis1100_remote_reg_write(struct SIS1100_softc* sc, u_int32_t offs,
    u_int32_t data, int locked)
{
    u_int32_t error;

    if (!locked) down(&sc->sem_hw);
    plxwritelocal0(sc, offs+0x800, data);
    mb();
    error=sis1100readreg(sc, prot_error);
    if (!locked) up(&sc->sem_hw);
    return error;
}

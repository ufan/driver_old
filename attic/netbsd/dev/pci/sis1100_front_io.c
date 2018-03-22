/* $ZEL: sis1100_front_io.c,v 1.2 2003/01/15 14:16:59 wuestner Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/sis1100_var.h>
#include <dev/pci/sis1100_sc.h>

/*
 * pseudoregister front_io:
 * bit write function        read function
 * 31
 * 30
 * 29
 * 28
 * 27  res pci_led_1         free
 * 26  res pci_led_0         free
 * 25  res pci_lemo_out_1    status pci_lemo_in_1
 * 24  res pci_lemo_out_0    status pci_lemo_in_0
 * 23  res vme_user_led      free
 * 22  res vme_lemo_out_3    status vme_lemo_in_3
 * 21  res vme_lemo_out_2    status vme_lemo_in_2
 * 20  res vme_lemo_out_1    status vme_lemo_in_1
 * 19  res vme_flat_out_4    status vme_flat_in_4
 * 18  res vme_flat_out_3    status vme_flat_in_3
 * 17  res vme_flat_out_2    status vme_flat_in_2
 * 16  res vme_flat_out_1    status vme_flat_in_1
 * 15
 * 14
 * 13
 * 12
 * 11  set pci_led_1         status pci_led_1
 * 10  set pci_led_0         status pci_led_0
 *  9  set pci_lemo_out_1    status pci_lemo_out_1
 *  8  set pci_lemo_out_0    status pci_lemo_out_0
 *  7  set vme_user_led      status vme_user_led
 *  6  set vme_lemo_out_3    status vme_lemo_out_3
 *  5  set vme_lemo_out_2    status vme_lemo_out_2
 *  4  set vme_lemo_out_1    status vme_lemo_out_1
 *  3  set vme_flat_out_4    status vme_flat_out_4
 *  2  set vme_flat_out_3    status vme_flat_out_3
 *  1  set vme_flat_out_2    status vme_flat_out_2
 *  0  set vme_flat_out_1    status vme_flat_out_1
 */
/*
 * set_vme_flat_out_?=0x0000000f
 * res_vme_flat_out_?=0x000f0000
 * set_vme_lemo_out_?=0x00000030
 * res_vme_lemo_out_?=0x00300000
 * set_vme_user_led  =0x00000080
 * res_vme_user_led  =0x00800000
 * set_pci_lemo_out_?=0x00000300
 * res_pci_lemo_out_?=0x03000000
 * set_pci_led_?     =0x00000c00
 * res_pci_led_?     =0x0c000000
 */

int
sis1100_front_io(struct SIS1100_softc* sc, u_int32_t* data, int locked)
{
    u_int32_t opt1100, io3100, st3100, _data;

    if (!locked) lockmgr(&sc->sem_hw, LK_EXCLUSIVE, 0);

    opt1100=sis1100readreg(sc, opt_csr);
    /* XXX no error handling yet */
    if (sc->remote_hw==sis1100_hw_vme) {
        io3100=plxreadlocal0(sc, ofs(struct sis3100_reg, in_out)+0x800);
        st3100=plxreadlocal0(sc, ofs(struct sis3100_reg, vme_master_sc)+0x800);
    } else {
        io3100=0;
        st3100=0;
    }
    _data = (io3100&0x7f007f) |    /* 3100 flat/lemo in/out */
            (st3100&0x80) |        /* 3100 user led */
            ((opt1100&0xf0)<<4) |  /* 1100 lemo out and led */
            ((opt1100&0x300)<<16); /* 1100 lemo in */

    opt1100&=0xff;
    opt1100&=~(*data>>20) & 0xf0;
    opt1100|=(*data>>4) & 0xf0;
    io3100=*data & 0x007f007f;
    st3100=*data & 0x00800080;

    sis1100writereg(sc, opt_csr, opt1100);
    if (sc->remote_hw==sis1100_hw_vme) {
        if (io3100)
            plxwritelocal0(sc, ofs(struct sis3100_reg, in_out)+0x800, io3100);
        if (st3100)
            plxwritelocal0(sc, ofs(struct sis3100_reg, vme_master_sc)+0x800, st3100);
    }
    if (!locked) lockmgr(&sc->sem_hw, LK_RELEASE, 0);
    *data=_data;
    return 0;
}

int
sis1100_front_pulse(struct SIS1100_softc* sc, u_int32_t* data, int locked)
{
    u_int32_t io3100;

    if (sc->remote_hw!=sis1100_hw_vme) return -ENXIO;

    io3100=(*data<<24) & 0x7f000000;

    if (!locked) lockmgr(&sc->sem_hw, LK_EXCLUSIVE, 0);
    plxwritelocal0(sc, ofs(struct sis3100_reg, in_out)+0x800, io3100);
    if (!locked) lockmgr(&sc->sem_hw, LK_RELEASE, 0);

    return 0;
}

int
sis1100_front_latch(struct SIS1100_softc* sc, u_int32_t* data, int locked)
{
    u_int32_t latch, _data;

    if (sc->remote_hw!=sis1100_hw_vme) return -ENXIO;

    latch=(*data<<24) & 0xff000000;
    if (!locked) lockmgr(&sc->sem_hw, LK_EXCLUSIVE, 0);
    _data=plxreadlocal0(sc, ofs(struct sis3100_reg, in_latch_irq)+0x800);
    plxwritelocal0(sc, ofs(struct sis3100_reg, in_latch_irq)+0x800, latch);
    if (!locked) lockmgr(&sc->sem_hw, LK_RELEASE, 0);

    *data=(_data>>24) & 0xff;
    return 0;
}

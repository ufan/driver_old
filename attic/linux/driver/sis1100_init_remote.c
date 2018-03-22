/* $ZEL: sis1100_init_remote.c,v 1.13 2003/01/09 12:13:16 wuestner Exp $ */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <asm/byteorder.h>

#include <dev/pci/sis1100_sc.h>

char* rnames[]={"PCI", "VME", "CAMAC"};

static void
remote_is_big_endian(struct SIS1100_softc* sc, int big)
{
    /* bigendian here only means that the interface will swap all data words */
    u_int32_t tmp;

    sis1100writereg(sc, cr, 8);
    tmp=plxreadreg(sc, BIGEND_LMISC_PROT_AREA);
#if defined(__LITTLE_ENDIAN)
    printk(KERN_INFO "little endian\n");
    if (big) tmp|=(1<<7); else tmp&=~(1<<7);
#elif defined(__BIG_ENDIAN)
    printk(KERN_INFO "big endian\n");
    if (big) tmp&=~(1<<7); else tmp|=(1<<7);
#else
#    error UNKNOWN ENDIAN
#endif
    plxwritereg(sc, BIGEND_LMISC_PROT_AREA, tmp);
}

void
sis1100_init_remote(struct SIS1100_softc* sc)
{
    u_int32_t ident, error, balance, typ, hv, fk, fv;
    int res;

    down(&sc->sem_hw);

    /*sis1100writereg(sc, cr, cr_rem_reset);*/ /* reset remote */
    sis1100_flush_fifo(sc, "init_remote" , 0); /* clear local fifo */
    sis1100writereg(sc, p_balance, 0);
    sis1100readreg(sc, prot_error);

    ident=plxreadlocal0(sc, 0x800);
    error=sis1100readreg(sc, prot_error);
    balance=sis1100readreg(sc, p_balance);
    up(&sc->sem_hw);

    if (error || balance) {
        printk(KERN_ERR "SIS1100[%d]: error reading remote ident\n", sc->unit);
        printk(KERN_ERR "error=0x%x balance=%d\n", error, balance);
        sis1100_flush_fifo(sc, "after reading ident" , 0); /* clear local fifo */
        /*sis1100_dump_glink_status(sc, "init remote");*/
        return;
    }

    typ=ident&0xff;
    hv=(ident>>8)&0xff;
    fk=(ident>>16)&0xff;
    fv=(ident>>24)&0xff;
    printk(KERN_INFO "SIS1100[%d]:%s: remote ident: 0x%08x\n",
            sc->unit, sc->pcidev->slot_name, ident);
    if ((typ>0) && (typ<4))
        printk(KERN_INFO "SIS1100[%d]:%s: remote is %s\n",
                sc->unit, sc->pcidev->slot_name, rnames[typ-1]);
    else
        printk(KERN_ERR "SIS1100[%d]:%s: unknown remote type %d\n",
                sc->unit, sc->pcidev->slot_name, ident&0xff);
    printk(KERN_INFO "SIS1100[%d]: remote HW_ver %d FW_code %d FW_ver %d\n",
                sc->unit, hv, fk, fv);
    sc->remote_ident=ident;

    switch (typ) {
        case 1: /* PCI */
            remote_is_big_endian(sc, 1); /* remote side has to swap */
            res=sis1100rem_init(sc);
            break;
        case 2: /* VME */
            remote_is_big_endian(sc, 1); /* VME is big_endian */
            res=sis3100rem_init(sc);
            break;
        case 3: /* CAMAC */
            remote_is_big_endian(sc, 0); /* CAMAC is unknown */
            res=sis5100rem_init(sc);
            break;
        default:
            printk("SIS1100[%d]: remote device type not (yet?) supported.\n",
                    sc->unit);
            res=-1;
            break;
    }
    if (res) return;

    sc->old_remote_hw=sc->remote_hw;
    sc->remote_hw=typ;
}

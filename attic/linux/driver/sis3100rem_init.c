/* $ZEL: sis3100rem_init.c,v 1.1 2003/01/09 12:16:05 wuestner Exp $ */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/pci.h>

#include <dev/pci/sis1100_sc.h>

static int
sis3100_dsp_present(struct SIS1100_softc* sc)
{
    u_int32_t dsp_sc;
    int res;

    res=sis3100readremreg(sc, dsp_sc, &dsp_sc, 0);
    if (res) {
        printk(KERN_INFO "SIS1100/3100[%d]: read dsp_sc: res=%d\n", sc->unit, res);
        return 0;
    }
    return !!(dsp_sc&sis3100_dsp_available);
}

static void
sis3100_dump_timeouts(struct SIS1100_softc* sc)
{
    u_int32_t stat;
    int berr_timer, long_timer, berr_time, long_time;

    sis3100readremreg(sc, vme_master_sc, &stat, 0);

    berr_timer=(stat>>14)&3;
    long_timer=(stat>>12)&3;
    berr_time=0;
    long_time=0;
    switch (berr_timer) {
        case 0: berr_time=1250; break;
        case 1: berr_time=6250; break;
        case 2: berr_time=12500; break;
        case 3: berr_time=100000; break;
    }
    switch (long_timer) {
        case 0: long_time=1; break;
        case 1: long_time=10; break;
        case 2: long_time=50; break;
        case 3: long_time=100; break;
    }
    printk("SIS1100/3100[%d]: berr_time=%d ns\n", sc->unit, berr_time);
    printk("SIS1100/3100[%d]: long_time=%d ms\n", sc->unit, long_time);
}

static void
sis3100_set_timeouts(struct SIS1100_softc* sc, int berr, int arb)
/* berr and arb in terms of 10**-6 s */
{
    int berr_timer, long_timer;
    u_int32_t bits;

    if (berr>12)
        berr_timer=3;
    else if (berr>6)
        berr_timer=2;
    else if (berr>1)
        berr_timer=1;
    else 
        berr_timer=0;

    if (arb>50000)
        long_timer=3;
    else if (arb>10000)
        long_timer=2;
    else if (arb>1000)
        long_timer=1;
    else 
        long_timer=0;

    bits=(long_timer|(berr_timer<<2))<<12;
    bits|=(~bits<<16)&0xf0000000;
    sis3100writeremreg(sc, vme_master_sc, bits, 0);
}

int
sis3100rem_init(struct SIS1100_softc* sc)
{
#define MIN_FV 3
#define MAX_FV 5
    u_int32_t hv, fk, fv, stat;

    hv=(sc->remote_ident>>8)&0xff;
    fk=(sc->remote_ident>>16)&0xff;
    fv=(sc->remote_ident>>24)&0xff;

    switch (sc->remote_ident&0x00ffff00) {
    case 0x00010100:
        if (fv<MIN_FV) {
            printk(KERN_ERR "SIS1100/3100[%d]: remote firmware version too old;"
                    " at least version %d is required.\n",
                    sc->unit, MIN_FV);
            return -1;
        }
        if (fv>MAX_FV) {
            printk(KERN_WARNING "SIS1100/3100[%d]: Driver not tested with"
                    " remote firmware versions greater than %d.\n",
                    sc->unit, MAX_FV);
        }
        break;
    default:
        printk(KERN_ERR "SIS1100/3100[%d]: remote hw/fw type not supported\n",
                sc->unit);
        return -1;
    }

    if (sis3100_init_sdram(sc)<0)
        return -1;
    printk(KERN_INFO "SIS1100/3100[%d]: size of SDRAM: 0x%llx (%lld MByte)\n",
        sc->unit, sc->ram_size, sc->ram_size>>20);

    sc->dsp_present=sis3100_dsp_present(sc);
    printk(KERN_INFO "SIS1100/3100[%d]: DSP is %spresent\n",
        sc->unit, sc->dsp_present?"":"not ");

    down(&sc->sem_hw);
    sis3100writeremreg(sc, vme_irq_sc, 0x00fe0001, 1); /* disable VME IRQs */
    sis3100readremreg(sc, vme_master_sc, &stat, 1);
    up(&sc->sem_hw);
    printk(KERN_INFO "SIS1100/3100[%d]: remote stat=0x%08x\n", sc->unit, stat);
    if (!(stat&vme_system_controller)) {
        printk(KERN_WARNING "SIS1100/3100[%d]: System Clock NOT enabled!\n",
                sc->unit);
    }
    sis3100_set_timeouts(sc, 1, 1000);
    sis3100_dump_timeouts(sc);

    return 0;
}

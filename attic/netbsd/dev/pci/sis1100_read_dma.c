/* $ZEL: sis1100_read_dma.c,v 1.18 2003/01/15 14:17:02 wuestner Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <vm/vm.h>
#include <uvm/uvm_extern.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/sis1100_var.h>
#include <dev/pci/sis1100_sc.h>

#include <dev/pci/sis1100_var.h>

static ssize_t
_sis1100_read_dma(
    struct SIS1100_softc* sc,
    struct SIS1100_fdata* fd,
    u_int32_t addr,           /* VME or SDRAM address */
    int32_t am,               /* address modifier, not used if <0 */
    u_int32_t size,           /* datasize must be 4 for DMA but is not checked*/
    int space,                /* remote space (1,2: VME; 6: SDRAM) */
    int fifo_mode,
    size_t count,             /* bytes to be transferred */
    u_int8_t* data,           /* destination (user virtual address) */
    int* prot_error
    )
{
    int res, i, aborted=0, s;
    u_int32_t head, tmp, __count=0;
    struct plx9054_dmadesc *dd;
    u_int32_t nextptr;

    if (count>MAX_DMA_LEN) count=MAX_DMA_LEN;
    res = uvm_vslock(fd->p, data, count, VM_PROT_READ|VM_PROT_WRITE);
    if (res) {
        printf("%s: uvm_vslock failed\n", sc->sc_dev.dv_xname);
        return (res);
    }

    res=bus_dmamap_load(sc->sc_dma.dmat, sc->sc_dma.userdma,
             data, count, fd->p, BUS_DMA_NOWAIT);
    if (res) {
        uvm_vsunlock(fd->p, data, count);
        printf("%s: bus_dmamap_load failed\n", sc->sc_dev.dv_xname);
        return res;
    }
    dd = sc->sc_dma.descs;
    nextptr = 0x0000000a;
    for (i = sc->sc_dma.userdma->dm_nsegs - 1; i >= 0; i--) {
        dd[i].pcistart = sc->sc_dma.userdma->dm_segs[i].ds_addr;
        if (dd[i].pcistart&0xc0000000) {
            printf("read: pcistart[%d]=0x%08x\n", i, dd[i].pcistart);
        }
        dd[i].size = sc->sc_dma.userdma->dm_segs[i].ds_len;
        dd[i].localstart = 0;
        dd[i].next = nextptr;
        nextptr = (sc->sc_dma.descdma->dm_segs[0].ds_addr +
                i * sizeof(struct plx9054_dmadesc)) | 9;
    }

/* prepare PLX */
    plxwritereg(sc, DMACSR0_DMACSR1, 1<<3); /* clear irq */
    plxwritereg(sc, DMAMODE0, 0x43|(1<<7)|(1<<8)|(1<<9)|(1<<10)|(1<<11)|
        (1<<12)|(1<<14)|(1<<17));
    plxwritereg(sc, DMADPR0, nextptr);

    tmp=plxreadreg(sc, BIGEND_LMISC_PROT_AREA);
    if (fd->big_endian)
        tmp|=(1<<7); /* big endian */
    else
        tmp&=~(1<<7); /* little endian */
    plxwritereg(sc, BIGEND_LMISC_PROT_AREA, tmp);

/* prepare add on logic */
    /* 4 Byte, local space 2, BT, EOT, start with t_adl */
    head=0x0f80A002|(space&0x3f)<<16;
    if (am>=0) {
        head|=0x800;
        sis1100writereg(sc, t_am, am);
    }
    if (fifo_mode) head|=0x4000;
    sis1100writereg(sc, t_hdr, head);
    /*wmb();*/
    sis1100writereg(sc, t_dal, count);

    sis1100writereg(sc, d0_bc, 0);
    sis1100writereg(sc, d0_bc_buf, 0);
    sis1100writereg(sc, d0_bc_blen, 0);

    sis1100writereg(sc, p_balance, 0);

/* enable irq */
    sc->got_irqs=0;
    sis1100_enable_irq(sc, plxirq_dma0, irq_synch_chg|irq_s_xoff);

/* enable dma */
    plxwritereg(sc, DMACSR0_DMACSR1, 3);

/* start transfer and wait for dma*/
    s = splbio();
    sis1100writereg(sc, t_adl, addr);

    {
    int nochmal=0;
    do {
        res=0;
        nochmal=0;
        while (!(res||(sc->got_irqs&(got_dma0|got_xoff|got_sync)))) {
                res = tsleep(&sc->local_wait, PCATCH, "plxdma_r_1", 10*hz);
        }
        if (res==ERESTART) {
            res=0;
            nochmal=1;
            printf("restart\n");
        }
    } while (nochmal);
    }
    splx(s);
    if (res||(sc->got_irqs&(got_sync|got_xoff))) {
        aborted=0x300;
        if (res) {
            if (res==EWOULDBLOCK)
                printf( "%s: read_dma: timed out\n",
                        sc->sc_dev.dv_xname);
            else if (res==EINTR)
                printf( "%s: read_dma(1): interrupted; res=%d\n",
                        sc->sc_dev.dv_xname, res);
            else if (res==ERESTART)
                 printf( "%s: read_dma(1): interrupted; restart\n",
                        sc->sc_dev.dv_xname);
            else
                 printf( "%s: read_dma(1): res=%d\n",
                        sc->sc_dev.dv_xname, res);
            aborted|=1;
        }
        if (sc->got_irqs&got_sync) {
            printf("%s: read_dma: synchronisation lost\n",
                    sc->sc_dev.dv_xname);
            aborted|=2;
        }
        if (sc->got_irqs&got_xoff) {
            printf("%s: read_dma: got xoff (irqs=0x%04x)\n",
                    sc->sc_dev.dv_xname, sc->got_irqs);
            aborted|=4;
        }
        sis1100writereg(sc, sr, 0x80000000);
    } else {
        __count=sis1100readreg(sc, d0_bc);
        if (!__count) { /* error reading first word from VME */
            /* send EOT to dma channel */
            sis1100writereg(sc, sr, 0x80000000);
            printf("%s: read_dma: count=0\n", sc->sc_dev.dv_xname);
            aborted|=8;
        }
    }

    sis1100_disable_irq(sc, plxirq_dma0, irq_s_xoff|irq_prot_end);
    plxwritereg(sc, DMACSR0_DMACSR1, 0);

    uvm_vsunlock(fd->p, data, count);

    *prot_error=sis1100readreg(sc, prot_error);

    if (aborted) {
        dump_glink_status(sc, "after abort", 1);
        printf("prot_error=0x%x; aborted=0x%x, __count=%d\n",
            *prot_error, aborted, __count);
    }
    if (aborted) *prot_error=aborted;
    if ((*prot_error!=0) && ((*prot_error&0x200)!=0x200)) __count=-EIO;

    return __count;
}

ssize_t
sis1100_read_dma(
    struct SIS1100_softc* sc,
    struct SIS1100_fdata* fd,
    u_int32_t addr,           /* VME or SDRAM address */
    int32_t am,               /* address modifier, not used if <0 */
    u_int32_t size,           /* datasize must be 4 for DMA but is not checked*/
    int space,                /* remote space (1,2: VME; 6: SDRAM) */
    int fifo_mode,
    size_t count,             /* bytes to be transferred */
    u_int8_t* data,           /* destination (user virtual address) */
    int* prot_err
    )
{
    ssize_t res=1;
    size_t completed=0;

    *prot_err=0;

    if (!count) return 0;

    lockmgr(&sc->sem_hw, LK_EXCLUSIVE, 0);

    while (count && (res>0) && (*prot_err==0)) {
        res=_sis1100_read_dma(sc, fd, addr, am, size, space, fifo_mode,
                count, data, prot_err);

        if (res>0) {
            if (!fifo_mode) addr+=res;
            data+=res;
            completed+=res;
            count-=res;
        } else if (res==0) {
            printf("sis1100_read_dma: res=0\n");
        }
    }

    lockmgr(&sc->sem_hw, LK_RELEASE, 0);
    if (completed)
        return completed;
    else
        return res;
}

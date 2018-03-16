/* $ZEL: sis1100_read_dma_netbsd.c,v 1.7 2007/02/22 19:41:20 trusov Exp $ */

/*
 * Copyright (c) 2001-2004
 * 	Matthias Drochner, Peter Wuestner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "sis1100_sc.h"

int
_sis1100_read_dma(
    struct sis1100_softc* sc,
    struct sis1100_fdata* fd,
    u_int32_t addr,           /* VME or SDRAM address */
    int32_t am,               /* address modifier, not used if <0 */
    int size,                 /* datasize must be 4 for DMA but is not checked*/
    int space,                /* remote space (1,2: VME; 6: SDRAM) */
    int fifo_mode,
    size_t count,             /* words to be transferred */
                              /* count==0 is illegal */
    size_t* count_read,       /* words transferred */
    u_int8_t* data,           /* destination (user virtual address) */
    int* prot_error,          /* protocol error */
    int* may_be_more          /* more data may be available */
    )
{
    int res, i, aborted=0, s;
    u_int32_t head;
    struct plx9054_dmadesc *dd;
    u_int32_t nextptr;
    size_t bytes_read=0;

    count*=size;
    if (count>MAX_DMA_LEN) count=MAX_DMA_LEN;

    res = uvm_vslock(fd->p, data, count, VM_PROT_WRITE);
    if (res) {
        pINFO(sc, "_read_dma: uvm_vslock failed, res=%d", res);
        pINFO(sc, "_read_dma: data=%p, count=%d", data, count);
        return EFAULT;
    }

    res=bus_dmamap_load(sc->sc_dma.dmat, sc->sc_dma.userdma,
             data, count, fd->p, BUS_DMA_NOWAIT);
    if (res) {
        uvm_vsunlock(fd->p, data, count);
        pINFO(sc, "bus_dmamap_load failed");
        return res;
    }

    bus_dmamap_sync(sc->sc_dma.dmat, sc->sc_dma.userdma, 0, count,
        BUS_DMASYNC_PREREAD);

    dd = sc->sc_dma.descs;
    nextptr = 0x0000000a;
    for (i = sc->sc_dma.userdma->dm_nsegs - 1; i >= 0; i--) {
        dd[i].pcistart = htole32(sc->sc_dma.userdma->dm_segs[i].ds_addr);
        dd[i].size = htole32(sc->sc_dma.userdma->dm_segs[i].ds_len);
        dd[i].localstart = htole32(0);
        dd[i].next = htole32(nextptr);
        nextptr = (sc->sc_dma.descdma->dm_segs[0].ds_addr +
                i * sizeof(struct plx9054_dmadesc)) | 9;
    }

/* prepare PLX */
    plxwritereg(sc, DMACSR0_DMACSR1, 1<<3); /* clear irq */
    plxwritereg(sc, DMAMODE0, 0x43|(1<<7)|(1<<8)|(1<<9)|(1<<10)|(1<<11)|
        (1<<12)|(1<<14)|(1<<17));
    plxwritereg(sc, DMADPR0, nextptr);


/* prepare add on logic */
    /* 4 Byte, local space 2, BT, EOT, start with t_adl */
    head=0x0080A002|((0x00f00000<<size)&0x0f000000)|(space&0x3f)<<16;
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
    sis1100_enable_irq(sc, plxirq_dma0, irq_synch_chg|irq_prot_l_err);

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
        while (!(res||(sc->got_irqs&(got_dma0|got_sync|got_l_err)))) {
                res = tsleep(&sc->local_wait, PCATCH, "plxdma_r_1", 10*hz);
        }
        if (res==ERESTART) {
            res=0;
            nochmal=0;
            pINFO(sc, "DMA read restart");
        }
    } while (nochmal);
    }
    sis1100_disable_irq(sc, plxirq_dma0, irq_prot_l_err);
    splx(s);
    if (sc->got_irqs&(got_dma0|got_l_err)) { /* transfer complete or error */
        bytes_read=sis1100readreg(sc, d0_bc);
        *count_read=bytes_read/size;
        *may_be_more=bytes_read==count;
    } else /*(res||(sc->got_irqs&(got_sync)))*/ { /* fatal */
        *count_read=sis1100readreg(sc, d0_bc); /* XXX ??? */
        aborted=0x300;
        if (res) {
            if (res==EWOULDBLOCK)
                pINFO(sc, "read_dma: timed out");
            else if (res==EINTR)
                pINFO(sc, "read_dma(1): interrupted; res=%d", res);
            else if (res==ERESTART)
                pINFO(sc, "read_dma(1): interrupted; restart");
            else
                pINFO(sc, "read_dma(1): res=%d", res);
            aborted|=1;
        }
        if (sc->got_irqs&got_sync) {
            pINFO(sc, "read_dma: synchronisation lost");
            aborted|=2;
        }
    }
    if (!(sc->got_irqs&got_dma0)) {
        u_int32_t val;
        val=plxreadreg(sc, DMACSR0_DMACSR1);
        if (!(val&0x10)) { /* DMA not stopped yet; abort it */
            sis1100writereg(sc, sr, sr_abort_dma);
            do {
                val=plxreadreg(sc, DMACSR0_DMACSR1);
            } while (!(val&0x10));
        }
    }

    plxwritereg(sc, DMACSR0_DMACSR1, 0);

    bus_dmamap_sync(sc->sc_dma.dmat, sc->sc_dma.userdma, 0, count,
        BUS_DMASYNC_POSTREAD);

    bus_dmamap_unload(sc->sc_dma.dmat, sc->sc_dma.userdma);
    uvm_vsunlock(fd->p, data, count);

    {
    int c=0;
    do {
        *prot_error=sis1100readreg(sc, prot_error);
    } while ((*prot_error==0x5) && (c++<1000));
    if (c>1) pINFO(sc, "read_dma: c=%d prot_error=0x%x", c, *prot_error);
    }

    /* *eot=!!(sis1100readreg(sc, sr)&0x200); */

    if (aborted) {
        /*dump_glink_status(sc, "after abort", 1);*/
        pINFO(sc, "prot_error=0x%x; aborted=0x%x, count_read=%d\n",
            *prot_error, aborted, *count_read);
    }
    if (aborted) *prot_error=aborted;
    if ((*prot_error!=0) && ((*prot_error&0x200)!=0x200)) {
        pINFO(sc, "read_dma: prot_error=0x%x", *prot_error);
        res=EIO;
    }

    return res;
}

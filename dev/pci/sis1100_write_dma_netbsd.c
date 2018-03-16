/* $ZEL: sis1100_write_dma_netbsd.c,v 1.4 2005/07/07 14:15:43 wuestner Exp $ */

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

#include <dev/pci/sis1100_sc.h>

int
_sis1100_write_dma(
    struct sis1100_softc* sc,
    struct sis1100_fdata* fd,
    u_int32_t addr,           /* VME or SDRAM address */
    int32_t am,               /* address modifier, not used if <0 */
    int size,                 /* datasize must be 4 for DMA but is not checked*/
    int space,                /* remote space (1,2: VME; 6: SDRAM) */
    int fifo_mode,
    size_t count,             /* words to be transferred */
                              /* count==0 is illegal */
    size_t* count_written,    /* words transferred */
    const u_int8_t* data,     /* source (user virtual address) */
    int* prot_error
    )
{
    int res, i, aborted=0, s;
    u_int32_t head, tmp;
    u_int32_t la=addr&0x7fffffffU; /* local address */
/*
    sigset_t oldset;
    int err, offs;
*/
    struct plx9054_dmadesc *dd;
    u_int32_t nextptr;

    count*=size;
    if (count>MAX_DMA_LEN) count=MAX_DMA_LEN;

    if ((addr^(addr+count))&0x80000000U) count=0x80000000U-addr;
    *count_written=count/size;

    res = uvm_vslock(fd->p, (u_int8_t*)data, count, VM_PROT_READ);
    if (res)
        return res;

    res=bus_dmamap_load(sc->sc_dma.dmat, sc->sc_dma.userdma,
             (u_int8_t*)data, count, fd->p, BUS_DMA_WAITOK);
    if (res) {
        uvm_vsunlock(fd->p, (u_int8_t*)data, count);
        printf("%s: bus_dmamap_load failed\n", sc->sc_dev.dv_xname);
        return res;
    }

    dd = sc->sc_dma.descs;
    nextptr = 0x00000002;
    if (!fifo_mode)
        la += count;
#ifdef PLXDEBUG
    printf("dma: %d segs, size %ld\n",
            sc->sc_dma.userdma->dm_nsegs, sc->sc_dma.userdma->dm_mapsize);
#endif
    for (i = sc->sc_dma.userdma->dm_nsegs - 1; i >= 0; i--) {
        if (!fifo_mode)
                la -= sc->sc_dma.userdma->dm_segs[i].ds_len;

        dd[i].pcistart = sc->sc_dma.userdma->dm_segs[i].ds_addr;
        dd[i].size = sc->sc_dma.userdma->dm_segs[i].ds_len;
        dd[i].localstart = la;
        dd[i].next = nextptr;
#ifdef PLXDEBUG
        printf("desc[%d]: %x/%x/%x/%x\n", i,
                dd[i].pcistart, dd[i].size, dd[i].localstart, dd[i].next);
#endif
        nextptr = (sc->sc_dma.descdma->dm_segs[0].ds_addr +
                i * sizeof(struct plx9054_dmadesc)) | 1;
    }

/* prepare PLX */
    plxwritereg(sc, DMACSR0_DMACSR1, 1<<3); /* clear irq */
    plxwritereg(sc, DMAMODE0,
        0x43|(1<<7)|(1<<8)|(1<<9)|(1<<10)|(1<<14)|(1<<17)|
            (fifo_mode?(1<<11):0));
    plxwritereg(sc, DMADPR0, nextptr);

/* prepare add on logic */
    /* 4 Byte, local space 2, BT, EOT, start with t_adl */
    head=0x0f80A402|(space&0x3f)<<16;
    if (am>=0) {
        head|=0x800;
        sis1100writereg(sc, d_am, am);
    }
    if (fifo_mode) head|=0x4000;
    sis1100writereg(sc, d_hdr, head);
    /*wmb();*/
    sis1100writereg(sc, d_adl, addr); /* only bit 31 is valid */

    sis1100writereg(sc, d_bc, count);

    sis1100writereg(sc, p_balance, 0);
#if 0
    spin_lock_irq(&current->sigmask_lock);
    oldset = current->blocked;
    sigfillset(&current->blocked);
    sigdelset(&current->blocked, SIGKILL);
    /* dangerous, should be removed later */
    /*if (!sigismember(&oldset, SIGINT)) sigdelset(&current->blocked, SIGINT);*/
    recalc_sigpending(current);
    spin_unlock_irq(&current->sigmask_lock);
#endif
/* enable irq */
    /* irq_synch_chg and irq_prot_l_err should always be enabled */
    sc->got_irqs=0;
    sis1100_enable_irq(sc, 0,
        irq_prot_l_err|irq_synch_chg|irq_prot_end);

/* start transfer and wait for confirmation*/
    res=0;
    s = splbio();
    plxwritereg(sc, DMACSR0_DMACSR1, 3);
    while (!res && !(sc->got_irqs & (got_end|got_sync|got_l_err))) {
        res = tsleep(&sc->local_wait, PCATCH, "plxdma_w_1", 1*hz);
    }
    splx(s);

    if (sc->got_irqs&got_l_err) {
        printf("sis1100: irq_prot_l_err in write_dma, irqs=0x%04x\n",
            sc->got_irqs);
    }
    if (res|(sc->got_irqs&(got_sync))) {
        aborted=0x300;
        if (res) {
            if (res==EWOULDBLOCK)
                printf( "%s: write_dma: timed out\n",
                        sc->sc_dev.dv_xname);
            else if (res==EINTR)
                printf( "%s: write_dma(1): interrupted; res=%d\n",
                        sc->sc_dev.dv_xname, res);
            else if (res==ERESTART)
                 printf( "%s: write_dma(1): interrupted; restart\n",
                        sc->sc_dev.dv_xname);
            else
                 printf( "%s: write_dma(1): res=%d\n",
                        sc->sc_dev.dv_xname, res);
            aborted|=1;
        }
        if (sc->got_irqs&got_sync) {
            printf("%s: write_dma: synchronisation lost\n",
                    sc->sc_dev.dv_xname);
            aborted|=2;
        }
    }

    sis1100_disable_irq(sc, 0, irq_prot_end);
#if 0
    spin_lock_irq(&current->sigmask_lock);
    current->blocked = oldset;
    recalc_sigpending(current);
    spin_unlock_irq(&current->sigmask_lock);
#endif
    *prot_error=sis1100readreg(sc, prot_error);

    if (aborted) {
        *prot_error=aborted;
        res=EIO;
    } else if (*prot_error) {
        if (*prot_error&0x200) {
            u_int32_t addr;
            head=0x0f000002;
            addr = (int)&((struct sis3100_reg*)(0))->dma_write_counter;
            sis1100writereg(sc, t_hdr, head);
            sis1100writereg(sc, t_adl, addr);
            do {
	        tmp=sis1100readreg(sc, prot_error);
            } while (tmp==0x005);
            if (tmp!=0) {
                printf("%s: write_dma: "
                    "read count after error: prot_error=0x%03x\n",
                    sc->sc_dev.dv_xname, tmp);
                res=EIO;
            } else {
                *count_written=sis1100readreg(sc, tc_dal)/size;
            }
        } else {
            res=EIO;
        }
    }

    /*if (aborted) dump_glink_status(sc, "after abort", 1);*/

    uvm_vsunlock(fd->p, (u_int8_t*)data, count);

    return res;
}

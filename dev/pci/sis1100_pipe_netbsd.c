/* $ZEL: sis1100_pipe_netbsd.c,v 1.3 2009/08/31 15:26:17 wuestner Exp $ */

/*
 * Copyright (c) 2001-2004
 * 	Peter Wuestner.  All rights reserved.
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
sis1100_read_pipe(struct sis1100_softc* sc, struct sis1100_pipe* control)
{
    struct sis1100_pipelist* list;
    struct mmapdma dmabuf;
    int rsegs, i, error, balance=0, res=0, num, s;

    list=malloc(control->num*sizeof(struct sis1100_pipelist),
            M_IOCTLOPS, M_WAITOK);
    if (!list) return ENOMEM;

    if (copyin(control->list, list,
            control->num*sizeof(struct sis1100_pipelist))) {
        free(list, M_IOCTLOPS);
    	res=EFAULT;
        goto raus_malloc;
    }

    /* how many bytes do we have to read? */
/*
    dmabuf.size=0;
    for (i=0; i<control->num; i++) {
        if (!(list[i].head&0x400))
            dmabuf.size+=sizeof(u_int32_t);
    }
*/
    dmabuf.size=control->num;

    dmabuf.dmat=sc->sc_dmat;
    res=bus_dmamem_alloc(dmabuf.dmat, dmabuf.size, 0, 0,
         &dmabuf.segs, 1, &rsegs, BUS_DMA_WAITOK);
    if (res) {
        pINFO(sc, "pipe: dmamem_alloc(%ld) failed", dmabuf.size);
        goto raus_malloc;
    }
    res=bus_dmamem_map(dmabuf.dmat, &dmabuf.segs, 1,
        dmabuf.size, &dmabuf.kva,
        BUS_DMA_WAITOK|BUS_DMA_COHERENT);
    if (res) {
        pINFO(sc, "pipe: bus_dmamem_map(%ld) failed", dmabuf.size);
        goto raus_bus_dmamem_alloc;
    }
    res=bus_dmamap_create(dmabuf.dmat, dmabuf.size, 1,
             dmabuf.size, 0, BUS_DMA_WAITOK, &dmabuf.dm);
    if (res) {
        pINFO(sc, "pipe: bus_dmamap_create(%ld) failed", dmabuf.size);
        goto raus_bus_dmamem_map;
    }
    res=bus_dmamap_load(dmabuf.dmat, dmabuf.dm, dmabuf.kva,
             dmabuf.size, NULL, BUS_DMA_WAITOK);
    if (res) {
        pINFO(sc, "pipe: bus_dmamap_load(%ld) failed", dmabuf.size);
        goto raus_bus_dmamap_create;
    }

    bus_dmamap_sync(dmabuf.dmat, dmabuf.dm, 0, dmabuf.size, BUS_DMASYNC_PREREAD);
    SEM_LOCK(sc->sem_hw);
    sis1100writereg(sc, rd_pipe_buf, dmabuf.segs.ds_addr);
    sis1100writereg(sc, rd_pipe_blen, dmabuf.size);

    sis1100_disable_irq(sc, 0, irq_prot_end);

    sis1100writereg(sc, t_hdr, 0); /* avoid premature start */
    for (i=0; i<control->num; i++) {
    	u_int32_t head;

    	sis1100writereg(sc, t_am, list[i].am);
    	sis1100writereg(sc, t_adl, list[i].addr);

    	head=(list[i].head&0x0f3f0400) /* be, remote space and w/r */
                          |0x00400001; /* local space 1, am, start */
 
    	if (list[i].head&0x400) { /* write request */
    	    sis1100writereg(sc, t_dal, list[i].data);
	    head&=~0x00400000; /* no pipeline mode */
    	}
    	sis1100writereg(sc, t_hdr, head);
    }

    sc->got_irqs=0;
    sis1100_enable_irq(sc, 0, irq_prot_end);

    s = splbio();
    while (!(res||((balance=sis1100readreg(sc, p_balance))==0))) {
            res = tsleep(&sc->local_wait, PCATCH, "pipe", 10*hz);
    }
    splx(s);

    sis1100_disable_irq(sc, 0, irq_prot_end);

    error=sis1100readreg(sc, prot_error);
    if (!balance) sis1100writereg(sc, p_balance, 0);

    if (error||res) {
    	/*printk(KERN_INFO
    	    "sis1100_read_pipe: error=0x%0x, res=%d\n", error, res);
	dump_glink_status(sc, "after pipe", 1);*/
    	sis1100_flush_fifo(sc, "pipe", 1);
    }
    num=dmabuf.size-sis1100readreg(sc, rd_pipe_blen);
    SEM_UNLOCK(sc->sem_hw);
    bus_dmamap_sync(dmabuf.dmat, dmabuf.dm, 0, dmabuf.size, BUS_DMASYNC_POSTREAD);

    control->error=error;
    control->num=num;

    if (copyout(dmabuf.kva, control->data, num)) {
    	res=EFAULT;
        goto raus_bus_dmamap_load;
    }

raus_bus_dmamap_load:
    bus_dmamap_unload(dmabuf.dmat, dmabuf.dm);
raus_bus_dmamap_create:
    bus_dmamap_destroy(dmabuf.dmat, dmabuf.dm);
raus_bus_dmamem_map:
    bus_dmamem_unmap(dmabuf.dmat, dmabuf.kva, dmabuf.size);
raus_bus_dmamem_alloc:
    bus_dmamem_free(dmabuf.dmat, &dmabuf.segs, 1);
raus_malloc:
    free(list, M_IOCTLOPS);

    return res;
}

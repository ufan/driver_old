/* $ZEL: sis1100_ddma_map_netbsd.c,v 1.1 2005/07/07 14:19:18 wuestner Exp $ */

/*
 * Copyright (c) 2005
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

#define DEBUG
#include "sis1100_sc.h"

void
sis1100_ddma_zero(struct demand_dma_block* block)
{
    block->uaddr=0;
    block->descdma=0;
    block->userdma=0;
    block->descs=0;
    block->descsegs=0;
    block->rsegs=0;
}

void
sis1100_ddma_unmap_block(struct sis1100_softc *sc,
        struct demand_dma_block* block)
{
    bus_dma_tag_t dmat = sc->sc_dmat;

    pINFO(sc, "ddma_unmap_block");

    if (block->descdma) {
        if (block->descdma->dm_mapsize) {
            /*printf("dma_unmap: dmamap_unload %p\n", block->descdma);*/
            bus_dmamap_unload(dmat, block->descdma);
        }
        /*printf("dma_unmap: dmamap_destroy %p\n", block->descdma);*/
        bus_dmamap_destroy(dmat, block->descdma);
    }

    if (block->descs) {
        /*printf("dma_unmap: dmamem_unmap %p\n", block->descs);*/
        bus_dmamem_unmap(dmat, (caddr_t)block->descs, block->nsegs*16);
    }

    if (block->descsegs) {
        if (block->rsegs) {
            /*printf("dma_unmap: dmamem_free %p\n", block->descsegs);*/
            bus_dmamem_free(dmat, block->descsegs, block->rsegs);
        }
        /*printf("dma_unmap: free %p\n", block->descsegs);*/
        free(block->descsegs, M_TEMP);
    }

    if (block->userdma) {
        if (block->userdma->dm_mapsize) {
            /*printf("dma_unmap: dmamap_unload %p\n", block->userdma);*/
            bus_dmamap_unload(dmat, block->userdma);
        }
        /*printf("dma_unmap: dmamap_destroy %p\n", block->userdma);*/
        bus_dmamap_destroy(dmat, block->userdma);
    }

    if (block->uaddr) {
        /*printf("dma_unmap: uvm_vsunlock %p\n", block->uaddr);*/
        uvm_vsunlock(block->p, block->uaddr, block->size);
    }

    pINFO(sc, "ddma_unmap_block OK.");
}

int
sis1100_ddma_map_block(struct sis1100_softc *sc,
    struct demand_dma_block* block)
{
    bus_dma_tag_t dmat = sc->sc_dmat;
    int dpseg, res;
/*
    pINFO(sc, "dma_map_block: size=%lld addr=%p",
            (unsigned long long)block->size, block->uaddr);
*/
    /* wire the user pages for DMA */
    res = uvm_vslock(block->p, block->uaddr, block->size,
            VM_PROT_READ|VM_PROT_WRITE);
    if (res) {
        pINFO(sc, "sis1100_ddma_map: uvm_vslock failed, res=%d", res);
        return EFAULT;
    }

    /* number of user pages (we require start and end at page boundaries) */
    block->nsegs = block->size/NBPG;
/*
    pERROR(sc, "dma_map: nsegs=%d dpseg=%d dsegs=%d", block->nsegs, dpseg,
            block->dsegs);
*/
    /* allocate a DMA handle for the user pages */
    res = bus_dmamap_create(dmat, block->size, block->nsegs, block->size, 0,
            BUS_DMA_WAITOK|BUS_DMA_ALLOCNOW, &block->userdma);
    if (res) {
        pINFO(sc, "dma_map: bus_dmamap_create failed");
        return res;
    }
/*
    printf("dmamap_create block->userdma=%p\n", block->userdma);
    printf("       _dm_size=%ld\n", block->userdma->_dm_size);
    printf("       _dm_segcnt=%d\n", block->userdma->_dm_segcnt);
    printf("       _dm_maxsegsz=%ld\n", block->userdma->_dm_maxsegsz);
    printf("       &dm_segs=%p\n", &block->userdma->dm_segs[0]);
*/
    /* load the DMA handle with user pages */
    res=bus_dmamap_load(dmat, block->userdma, block->uaddr, block->size,
            block->p, BUS_DMA_WAITOK);
    if (res) {
        pINFO(sc, "bus_dmamap_load failed");
        return res;
    }
/*
    printf("dmamap_load block->userdma=%p\n", block->userdma);
    printf("       dm_mapsize=%ld\n", block->userdma->dm_mapsize);
    printf("       dm_nsegs=%d\n", block->userdma->dm_nsegs);
    for (i=0; i<block->userdma->dm_nsegs; i++) {
        printf("       dm_segs[%d].ds_addr=0x%08lx\n", i, block->userdma->dm_segs[i].ds_addr);
        printf("       dm_segs[%d].ds_len =%ld\n", i, block->userdma->dm_segs[i].ds_len);
    }
*/

    /* pages needed for desriptors */
    dpseg = NBPG / sizeof(struct plx9054_dmadesc); /* desriptors per page */
    block->dsegs = block->userdma->dm_nsegs / dpseg +1;

    /* allocate memory for the segment descriptors of the PLX descriptors */
    block->descsegs=malloc(block->dsegs*sizeof(bus_dma_segment_t*), 0, M_TEMP);
    if (block->descsegs==0) {
        pINFO(sc, "dma_map: malloc failed");
        return ENOMEM;
    }
/*
    printf("dma_map: malloc %p size=%d\n", block->descsegs,
            block->dsegs*sizeof(bus_dma_segment_t*));
*/
    res = bus_dmamem_alloc(dmat, block->dsegs*NBPG, 0, 0, block->descsegs,
            block->dsegs, &block->rsegs, BUS_DMA_WAITOK);
    if (res) {
        pINFO(sc, "dma_map: bus_dmamem_alloc failed");
        return res;
    }
/*    printf("dmamem_alloc block->descsegs=%p\n", block->descsegs);
    printf("      rsegs=%d\n", block->rsegs);
    for (i=0; i<block->rsegs; i++) {
        printf("      descsegs[%d].ds_addr=0x%08lx\n", i, block->descsegs[i].ds_addr);
        printf("      descsegs[%d].ds_len=%ld\n", i, block->descsegs[i].ds_len);
    }
*/
    /* map it into kernel virtual address space */
    res = bus_dmamem_map(dmat, block->descsegs, block->rsegs, block->nsegs*16,
            (caddr_t*)&block->descs, BUS_DMA_WAITOK|BUS_DMA_COHERENT);
    if (res) {
        pINFO(sc, "dma_map: bus_dmamem_map failed");
        return res;
    }
/*
    printf("dmamem_map block->descs=%p\n", block->descs);
*/
    /* allocate a DMA handle for the PLX descriptors */
    res = bus_dmamap_create(dmat, block->dsegs*NBPG, block->dsegs,
            block->dsegs*NBPG, 0, BUS_DMA_WAITOK|BUS_DMA_ALLOCNOW,
            &block->descdma);
    if (res) {
        pINFO(sc, "dma_map: bus_dmamap_create failed");
        return res;
    }
/*
    printf("dmamap_create block->descdma=%p\n", block->descdma);
    printf("       _dm_size=%ld\n", block->descdma->_dm_size);
    printf("       _dm_segcnt=%d\n", block->descdma->_dm_segcnt);
    printf("       _dm_maxsegsz=%ld\n", block->descdma->_dm_maxsegsz);
    printf("       &dm_segs=%p\n", &block->descdma->dm_segs[0]);
*/
    /* load the DMA handle */
    res = bus_dmamap_load(dmat, block->descdma, block->descs, block->dsegs*NBPG,
            NULL, BUS_DMA_WAITOK);
    if (res) {
        pINFO(sc, "dma_map: bus_dmamap_load failed");
        return res;
    }
/*
    printf("dmamap_load block->descdma=%p\n", block->descdma);
    printf("       dm_mapsize=%ld\n", block->descdma->dm_mapsize);
    printf("       dm_nsegs=%d\n", block->descdma->dm_nsegs);
    for (i=0; i<block->descdma->dm_nsegs; i++) {
        printf("       dm_segs[%d].ds_addr=0x%08lx\n", i, block->descdma->dm_segs[i].ds_addr);
        printf("       dm_segs[%d].ds_len =%ld\n", i, block->descdma->dm_segs[i].ds_len);
    }
*/
    /* fill descriptor buffer for PLX */
    {
        int seg;
        char* virt_start=(char*)block->descs;
        size_t offs=0;

        bus_dma_segment_t* user_seg=block->userdma->dm_segs;
        bus_dma_segment_t* desc_seg=block->descdma->dm_segs;
        block->dmadpr0=desc_seg->ds_addr;
        /*pERROR(sc, "dmadpr0=%08x", block->dmadpr0);*/

        for (seg=0; seg<block->userdma->dm_nsegs; seg++) {
            u_int32_t next;

            struct plx9054_dmadesc* plx_desc =
                (struct plx9054_dmadesc*)(virt_start+offs);

            offs+=sizeof(struct plx9054_dmadesc);
            if (offs>=desc_seg->ds_len) {
                virt_start+=desc_seg->ds_len;
                offs=0;
                desc_seg++;
            }
            if (seg<block->userdma->dm_nsegs-1)
                next=(desc_seg->ds_addr+offs)|0x9;
            else
                next=0xb;

            plx_desc->pcistart=htole32(user_seg->ds_addr);
            plx_desc->size=htole32(user_seg->ds_len);
            plx_desc->localstart=htole32(0);
            plx_desc->next=htole32(next);

            user_seg++;
/*
            if (seg<3) {
                pERROR(sc, "seg=%d offs=0x%08llx", seg, (unsigned long long)offs);
                pERROR(sc, "desc      =%p",     plx_desc);
                pERROR(sc, "pcistart  =0x%08x", plx_desc->pcistart);
                pERROR(sc, "size      =0x%08x", plx_desc->size);
                pERROR(sc, "localstart=0x%08x", plx_desc->localstart);
                pERROR(sc, "next      =0x%08x", plx_desc->next);
            }
*/
        }
    }

    return 0;
}

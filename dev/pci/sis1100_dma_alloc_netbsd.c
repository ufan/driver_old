/* $ZEL: sis1100_dma_alloc_netbsd.c,v 1.6 2005/07/07 14:15:31 wuestner Exp $ */

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

#if !defined(__NetBSD__)
#error Invalid Operating System
#endif

int
sis1100_dma_alloc(struct sis1100_softc *sc, struct sis1100_fdata* fd,
    struct sis1100_dma_alloc* d)
{
    int rsegs, res;

printf("sis1100_dma_alloc!\n");
    if (fd->mmapdma.valid) {
        pINFO(sc, "mmapdma already valid");
        return EINVAL;
    }

    fd->mmapdma.dmat=sc->sc_dmat;
    fd->mmapdma.size=d->size;
    /*fd->mmapdma.off=i386_round_page(sc->reg_size);*/
    fd->mmapdma.off=(sc->reg_size+PGOFSET) & ~PGOFSET;

    res=bus_dmamem_alloc(fd->mmapdma.dmat, fd->mmapdma.size, 0, 0,
             &fd->mmapdma.segs, 1, &rsegs, BUS_DMA_WAITOK);
    if (res) {
        printf("%s: dmamem_alloc(%d) failed\n", sc->sc_dev.dv_xname, d->size);
        return res;
    }

    res=bus_dmamem_map(fd->mmapdma.dmat, &fd->mmapdma.segs, 1,
            fd->mmapdma.size, &fd->mmapdma.kva,
            BUS_DMA_WAITOK|BUS_DMA_COHERENT);
    if (res) {
        printf("%s: bus_dmamem_map(%ld) failed\n", sc->sc_dev.dv_xname,
                fd->mmapdma.size);
        return res;
    }

    res=bus_dmamap_create(fd->mmapdma.dmat, fd->mmapdma.size, 1,
             fd->mmapdma.size, 0, BUS_DMA_WAITOK,
             &fd->mmapdma.dm);
    if (res) {
        printf("%s: bus_dmamap_create(%ld) failed\n", sc->sc_dev.dv_xname,
                fd->mmapdma.size);
        return res;
    }

    res=bus_dmamap_load(fd->mmapdma.dmat, fd->mmapdma.dm, fd->mmapdma.kva,
             fd->mmapdma.size, NULL, BUS_DMA_WAITOK);
    if (res) {
        printf("%s: bus_dmamap_load(%ld) failed\n", sc->sc_dev.dv_xname,
                fd->mmapdma.size);
        return res;
    }
    fd->mmapdma.valid=1;
    d->size=fd->mmapdma.size;
    d->offset=fd->mmapdma.off;
    d->dma_addr=fd->mmapdma.segs.ds_addr;
    pINFO(sc, "%ld bytes for DMA mapped", fd->mmapdma.size);
    return 0;
}

int
sis1100_dma_free(struct sis1100_softc *sc, struct sis1100_fdata* fd,
    struct sis1100_dma_alloc* d)
{
printf("sis1100_dma_free!\n");
    if (!fd->mmapdma.valid) return 0;
    fd->mmapdma.valid=0;
    bus_dmamap_destroy(fd->mmapdma.dmat, fd->mmapdma.dm);
    bus_dmamem_unmap(fd->mmapdma.dmat, fd->mmapdma.kva, fd->mmapdma.size);
    bus_dmamem_free(fd->mmapdma.dmat, &fd->mmapdma.segs, 1);
    pINFO(sc, "%ld bytes for DMA unmapped", fd->mmapdma.size);
    return 0;
}

/* $ZEL: plx9054dma_netbsd.c,v 1.3 2005/07/07 14:15:29 wuestner Exp $ */

/*
 * Copyright (c) 2003-2004
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
plx9054_dmaalloc(struct plx9054dma* sc, int maxlen)
{
	bus_dma_tag_t dmat = sc->dmat;
	int res, rsegs;

	sc->nsegs = (maxlen + NBPG - 2) / NBPG + 1;
#ifdef PLXDEBUG
	printf("dma: allocating %d descriptors\n", sc->numdescs);
#endif

        /* allocate memory for the PLX descriptors */
	res = bus_dmamem_alloc(dmat, sc->nsegs*16, 16, 0, &sc->descsegs, 1,
			       &rsegs, 0);
	if (res) {
		printf("%s: bus_dmamem_alloc failed\n", sc->devname);
		return (res);
	}
printf("plx9054 dmamem_alloc\n");
        /* map it into kernel virtual address space */
	res = bus_dmamem_map(dmat, &sc->descsegs, 1, sc->nsegs * 16,
			     (caddr_t *)&sc->descs, 0);
	if (res) {
		printf("%s: bus_dmamem_map failed\n", sc->devname);
		return (res);
	}
printf("plx9054 dmamem_map: %p\n", sc->descs);

        /* allocate a DMA handle for the PLX descriptors */
	res = bus_dmamap_create(dmat, sc->nsegs*16, 1, sc->nsegs*16, 0,
                BUS_DMA_WAITOK|BUS_DMA_ALLOCNOW, &sc->descdma);
	if (res) {
		printf("%s: bus_dmamap_create failed\n", sc->devname);
		return (res);
	}
printf("plx9054 dmamap_create: %p\n", sc->descdma);
        /* load the DMA handle */
	res = bus_dmamap_load(dmat, sc->descdma, sc->descs, sc->nsegs * 16,
			      NULL, 0);
	if (res) {
		printf("%s: bus_dmamap_load failed\n", sc->devname);
		return (res);
	}
printf("plx9054 dmamap_load: %p\n", sc->descdma);

        /* allocate a DMA handle for the user pages */
        /* it will be loaded later */
	res = bus_dmamap_create(dmat, maxlen, sc->nsegs, maxlen, 0,
                BUS_DMA_WAITOK|BUS_DMA_ALLOCNOW, &sc->userdma);
	if (res) {
		printf("%s: bus_dmamap_create failed\n", sc->devname);
		return (res);
	}
printf("plx9054 dmamap_create: %p\n", sc->userdma);
        sc->userdma->dm_mapsize=0;
	return (0);
}

void
plx9054_dmafree(struct plx9054dma* sc)
{
	if (sc->userdma) {
printf("plx9054_free: dmamap_destroy %p\n", sc->userdma);
		bus_dmamap_destroy(sc->dmat, sc->userdma);
        }
	if (sc->descdma) {
printf("plx9054_free: dmamap_unload %p\n", sc->descdma);
                bus_dmamap_unload(sc->dmat, sc->descdma);
printf("plx9054_free: dmamap_destroy %p\n", sc->descdma);
		bus_dmamap_destroy(sc->dmat, sc->descdma);
        }
	if (sc->descs) {
printf("plx9054 dmamem_unmap: %p\n", sc->descs);
                bus_dmamem_unmap(sc->dmat, (caddr_t)sc->descs, sc->nsegs*16);
printf("plx9054_free: dmamem_free\n");
		bus_dmamem_free(sc->dmat, &sc->descsegs, 1);
        }
}

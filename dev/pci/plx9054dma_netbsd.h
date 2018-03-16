/* $ZEL: plx9054dma_netbsd.h,v 1.2 2004/05/27 23:10:16 wuestner Exp $ */

/*
 * Copyright (c) 2004
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

#ifndef _plx9054dma_h_
#define _plx9054dma_h_

struct plx9054dma {
        char *devname;
        bus_space_tag_t iot;
        bus_space_handle_t ioh;
        bus_dma_tag_t dmat;

        int nsegs, rsegs;
        bus_dmamap_t userdma;
        bus_dma_segment_t descsegs;
        bus_dma_segment_t segs;
        struct plx9054_dmadesc *descs; /* kva of descsegs */
        bus_dmamap_t descdma;
        caddr_t kva; /* kva of segs */
};

int plx9054_dmaalloc(struct plx9054dma* sc, int maxlen);
void plx9054_dmafree(struct plx9054dma* sc);

#endif

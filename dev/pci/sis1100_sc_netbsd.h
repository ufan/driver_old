/* $ZEL: sis1100_sc_netbsd.h,v 1.10 2008/01/17 15:55:07 wuestner Exp $ */

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

#ifndef _sis1100_sc_netbsd_h_
#define _sis1100_sc_netbsd_h_

#include "dev/pci/plx9054dma_netbsd.h"

#define MAX_DMA_LEN 524288 /* 1/2 MByte */

extern struct cfdriver sis1100cfdriver;

struct sis1100_softc;

struct mmapdma {
        bus_dma_tag_t dmat;
        int valid;
        off_t off;
        bus_size_t size;
        bus_dma_segment_t segs;
        bus_dmamap_t dm;
        caddr_t kva; /* kva of segs */
};

struct handlercommand {
    volatile enum handlercomm command;
    struct simplelock lock;
};

struct sis1100_fdata {
    struct list_head list;
/* OS specific*/
    struct proc *p;
/*common*/
    struct mmapdma mmapdma;
    size_t mindmalen_r, mindmalen_w;
    enum sis1100_hw_type old_remote_hw;
    enum sis1100_subdev subdev;
    int32_t vmespace_am;
    u_int32_t vmespace_datasize;
    int fifo_mode;
    int last_prot_err;
    int owned_irqs;
    pid_t pid;
    int sig;
};

/*
 * each block has to start at a page boundary and its length has to be a
 * multiple of the page size
 * all blocks have to have the same size
 * at least two blocks are required
 */
struct demand_dma_block {
    char* uaddr; /* user virtual address of this block */
    size_t size; /* size of this block */
    struct proc *p;

    int nsegs; /* number of mapped user pages */
    int dsegs; /* maximum number of segments for PLX descriptors */
    int rsegs; /* actual number of segments for PLX descriptors */
    bus_dmamap_t descdma; /* DMA handle for PLX descriptors */
    bus_dmamap_t userdma; /* DMA handle user pages */
    bus_dma_segment_t* descsegs; /* opaque addresses of descdma */
                                /* dsegs*sizeof(bus_dma_segment_t*) */
    struct plx9054_dmadesc* descs; /*kernel virtual address of PLX descriptors*/
                                /* bus addresses are in descdma->dm_segs */
    u_int32_t dmadpr0;
/*
    int used;
*/
    enum dmablockstatus status;
    int seq_write_start;
    int seq_write_end;
    int seq_signal;
    int seq_free;
};

/* demand_dma should be identical to the linux version */
struct demand_dma {
    struct lock sem;              /* protects this structure */
    struct simplelock spin;       /* protects is_blocked and block[].used */

    enum dmastatus status;        /* invalid | ready | running */
    struct sis1100_fdata *owner;
    
    char* uaddr; /* user virtual address of the first block */
    size_t size; /* size of ONE block of mapped user pages */
    int numblocks; /* number of blocks */
    struct demand_dma_block* block;
/*
    int active_block;
    int last_block;
*/
    int is_blocked;               /* all buffers are full, DMA is stopped */
    int writing_block;            /* block currently written by DMA or
                                     waiting until free */
    int reading_block;            /* last block given to user */
    int debug_sequence;           /* counter, incremented after each DMA-block */
};

struct sis1100_softc {
/* OS specific*/
    struct device sc_dev;
    pci_chipset_tag_t sc_pc;
    pcitag_t sc_pcitag;
    bus_dma_tag_t sc_dmat;

    bus_space_tag_t plx_t;
    bus_space_handle_t plx_h;
    bus_size_t plx_size;
    bus_addr_t plx_addr;

    bus_space_tag_t reg_t;
    bus_space_handle_t reg_h;
    bus_size_t reg_size;
    bus_addr_t reg_addr;

    bus_space_tag_t rem_t;
    bus_space_handle_t rem_h;
    bus_size_t rem_size;
    bus_addr_t rem_addr;

    void *sc_ih;

    struct plx9054dma sc_dma;

    struct simplelock lock_sc_inuse;      /* protects sc_inuse */
    struct proc* vmeirq_pp;
    int sc_inuse;
    struct selinfo sel;

/* OS specific definition but common use */
    struct lock sem_hw;                   /* protects hardware */
    struct lock sem_fdata_list;           /* protects fdata_list_head */
    struct lock sem_irqinfo;              /* protects irq_vects, pending_irqs
                                             and new_irqs */
    struct simplelock lock_intcsr;        /* protects INTCSR of PLX */
    struct simplelock lock_doorbell;      /* protects sc.doorbell */
    struct simplelock lock_lemo_status;   /* protects sc.lemo_status */
    void*/*struct simplelock*/ handler_wait; /* pending_irqs, remote_ok */
    struct simplelock local_wait;
    struct simplelock remoteirq_wait;
    struct callout link_up_timer;

/* common */
    struct sis1100_fdata* fdatalist[sis1100_MINORUTMASK+1];
    struct list_head fdata_list_head;
    u_int32_t local_ident, remote_ident;
    volatile enum sis1100_hw_type remote_hw, old_remote_hw;
    volatile u_int32_t doorbell;
    volatile u_int32_t lemo_status;
    volatile u_int32_t mbx0;
    volatile int got_irqs;
    struct irq_vects irq_vects[8];
    int pending_irqs, new_irqs; /* XXX id new_irqs necessary??? */
    struct handlercommand handlercommand;
    off_t ram_size;
    int dsp_present;
    int remote_endian; /* 0: little 1: big*/
    int user_wants_swap;
    u_int32_t last_opt_csr; /* used by handlercomm_lemo */
    struct demand_dma demand_dma;
    void (*plxirq_dma0_hook)(struct sis1100_softc*);

#if 0
    int dma_dac; /* use 64bit dual address cycle for dma */
    int no_dma;  /* even 32bit dma not available */
#endif
};

#define SIS1100CARD(dev) \
 ((minor(dev)&sis1100_MINORCARDMASK)>>sis1100_MINORCARDSHIFT)
#define SIS1100SC(dev) \
 ((struct sis1100_softc*)sis1100cfdriver.cd_devs[SIS1100CARD(dev)])
#define SIS1100FD(dev) \
 (struct sis1100_fdata*)((SIS1100SC(dev)->fdatalist)[minor(dev)&sis1100_MINORUTMASK])

#define _plxreadreg(sc, offset) \
    bus_space_read_4(sc->plx_t, sc->plx_h, offset)

#define _plxwritereg(sc, offset, val) \
    bus_space_write_4(sc->plx_t, sc->plx_h, offset, val)

#define plxreadlocal0(sc, offset) \
    bus_space_read_4(sc->reg_t, sc->reg_h, offset)

#define plxreadlocal0b(sc, offset) \
    bus_space_read_1(sc->reg_t, sc->reg_h, offset)

#define plxwritelocal0(sc, offset, val) \
    bus_space_write_4(sc->reg_t, sc->reg_h, offset, val)

#define plxwritelocal0b(sc, offset, val) \
    bus_space_write_1(sc->reg_t, sc->reg_h, offset, val)

#define plxrawreadlocal0(sc, offset) \
    bus_space_read_stream_4(sc->reg_t, sc->reg_h, offset)

#define plxrawwritelocal0(sc, offset, val) \
    bus_space_write_stream_4(sc->reg_t, sc->reg_h, offset, val)

#define rmb_plx() bus_space_barrier(sc->plx_t, sc->plx_h, 0, sc->plx_size, \
    BUS_SPACE_BARRIER_READ)
#define rmb_reg() bus_space_barrier(sc->reg_t, sc->reg_h, 0, sc->reg_size, \
    BUS_SPACE_BARRIER_READ)
#define wmb_plx() bus_space_barrier(sc->plx_t, sc->plx_h, 0, sc->plx_size, \
    BUS_SPACE_BARRIER_WRITE)
#define wmb_reg() bus_space_barrier(sc->reg_t, sc->reg_h, 0, sc->reg_size, \
    BUS_SPACE_BARRIER_WRITE)
#define mb_plx() bus_space_barrier(sc->plx_t, sc->plx_h, 0, sc->plx_size, \
    BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE)
#define mb_reg() bus_space_barrier(sc->reg_t, sc->reg_h, 0, sc->reg_size, \
    BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE)

#define pLOG(sc, level, fmt, arg...)                              \
    do {                                                          \
        struct sis1100_softc *_sc=sc;                             \
        if (_sc)                                                  \
            printf("%s: " fmt "\n", sc->sc_dev.dv_xname , ## arg) \
        else                                                      \
            printf("sis1100: " fmt "\n" , ## arg)                 \
    } while (0)


void sis1100_done(struct sis1100_softc*);
int sis1100_intr(void*);

int sis1100_open(dev_t, int, int, struct proc*);
int sis1100_close(dev_t, int, int, struct proc*);
int sis1100_ioctl(dev_t, u_long, caddr_t, int, struct proc*);
int sis1100_read(dev_t dev, struct uio* uio, int f);
int sis1100_write(dev_t dev, struct uio* uio, int f);
paddr_t sis1100_mmap(dev_t dev, off_t off, int prot);
void sis1100_irq_thread(void* data);
void sis1100_link_up_handler(void*);
int sis1100_poll(dev_t dev, int events, struct proc *p);

#endif

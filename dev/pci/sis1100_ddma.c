/* $ZEL: sis1100_ddma.c,v 1.3 2016/03/16 20:21:11 wuestner Exp $ */

/*
 * Copyright (c) 2005-2010
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
#include "zellvd_map.h"

/* start_dma may be called from interrupt! */
static void
start_dma(struct sis1100_softc *sc, struct demand_dma_block *block)
{
    DECLARE_SPINLOCKFLAGS(flags)
    u_int32_t intcsr;

    /* clear IRQ */
    plxwritereg(sc, DMACSR0_DMACSR1, 1<<3);

    /* enable IRQ */
    SPIN_LOCK_IRQSAVE(sc->lock_intcsr, flags);
    intcsr=plxreadreg(sc, INTCSR);
    plxwritereg(sc, INTCSR, intcsr|plxirq_dma0);
    SPIN_UNLOCK_IRQRESTORE(sc->lock_intcsr, flags);

    /* (re)start DMA */
    plxwritereg(sc, DMADPR0, block->dmadpr0|9);
    mb_plx();
    plxwritereg(sc, DMACSR0_DMACSR1, 3);
}


/* plxirq_dma0_hook is called from interrupt! */
static void
plxirq_dma0_hook(struct sis1100_softc *sc, struct timespec time)
{
    DECLARE_SPINLOCKFLAGS(flags)
    struct demand_dma* dma = &sc->demand_dma;
    struct demand_dma_block *block;

#if 0
    pERROR(sc, "plxirq_dma0_hook called");
#endif

    /* mark current DMA block as full */
    block=dma->block+dma->writing_block;
    /* sanity check */
    if (block->status!=dmablock_dma) {
        pERROR(sc, "plxirq_dma0_hook: current block %d has status %d",
            dma->writing_block, block->status);
        return;
    }
    block->status=dmablock_full;
    block->time=time;

    if (dma->writing_block==-2)
        pERROR(sc, "dma0_hook called with active dummyblock");

    /* prepare for next DMA
       because we change dma->blstat in competition to sis1100_ddma_mark
       and sis1100_ddma_stop we need the spinlock */
    SPIN_LOCK_IRQSAVE(dma->spin, flags);

    if (dma->blstat==ddmabl_aborting) {
        dma->writing_block=-2;
        block=&dma->dummyblock;
        block->status=dmablock_free;
        pERROR(sc, "dummyblock in use");
    } else {
        dma->writing_block=DMA_NEXT(dma->writing_block);
        block=dma->block+dma->writing_block;
    }

    if (block->status==dmablock_free) {
        block->status=dmablock_dma;
        start_dma(sc, block);
    } else {
#if 0
        pERROR(sc, "dma is now blocked");
#endif
        dma->blstat=ddmabl_blocked;
    }

    SPIN_UNLOCK_IRQRESTORE(dma->spin, flags);

    /* let irq_thread inform the user process */
    /* dma_sync is done in sis1100_irq_get() */
    SPIN_LOCK_IRQSAVE(sc->handlercommand.lock, flags);
    sc->handlercommand.command|=handlercomm_ddma;
    SPIN_UNLOCK_IRQRESTORE(sc->handlercommand.lock, flags);
    wake_up_process(sc->handler);
}

int
sis1100_ddma_start(struct sis1100_softc *sc, struct sis1100_fdata* fd)
{
    struct demand_dma* dma = &sc->demand_dma;
    u_int32_t dmamode;
    int i;

#if 0
    pERROR(sc, "ddma_start");
#endif

    mutex_lock(&dma->mux);
    /* is an other process using dma? */
    if ((dma->status!=ddma_invalid) && (dma->owner!=fd)) {
        mutex_unlock(&dma->mux);
        return EPERM;
    }
    if (dma->status==ddma_running) {
        mutex_unlock(&dma->mux);
        return EALREADY;
    }
    if (dma->status!=ddma_ready) {
        mutex_unlock(&dma->mux);
        return EINVAL;
    }

    /* declare all blocks as free */
    for (i=0; i<dma->numblocks; i++)
        dma->block[i].status=dmablock_free;
    dma->dummyblock.status=dmablock_free;

    dma->blstat=ddmabl_running;
    dma->reading_block=0;
    dma->writing_block=0;
    dma->block[0].status=dmablock_dma;

    /* prepare PLX */
    dmamode=3     /* bus width 32 bit */
        |(1<<6)   /* enabe TA#/READY# input */
        |(1<<7)   /* enable BTERM# input */
        |(1<<8)   /* enable loacal burst */
        |(1<<9)   /* scatter/gather mode */
        |(1<<10)  /* enabe "done interrupt" */
        |(1<<11)  /* local address is constant */
        |(1<<12)  /* demand mode */
        |(1<<17); /* routing DMA interrupt to PCI bus */
        /*|(1<<14)*/  /* enable DMA EOT# */
        /* EOT stops DMA after the first event! */
        if (sc->using_dac)
            dmamode|=(1<<18);

    plxwritereg(sc, DMAMODE0, dmamode);

    sc->plxirq_dma0_hook=plxirq_dma0_hook;

    /* enable remote access */
    sis1100writereg(sc, cr, cr_ready);

#if 0
    /* clear byte counter */
    sis1100writereg(sc, d0_bc, 0);
#endif

    start_dma(sc, dma->block+0);

    dma->status=ddma_running;
    mutex_unlock(&dma->mux);

    return 0;
}

int
sis1100_ddma_stop(struct sis1100_softc *sc, struct sis1100_fdata* fd,
        struct sis1100_ddma_stop* d)
{
    DECLARE_SPINLOCKFLAGS(flags)
    struct demand_dma* dma=&sc->demand_dma;

#if 0
    pERROR(sc, "ddma_stop");
#endif

    mutex_lock(&dma->mux);
    /* is an other process using dma? */
    if ((dma->status!=ddma_invalid)&&(dma->owner!=fd)){
        mutex_unlock(&dma->mux);        
        return EPERM;
    }
    if (dma->status!=ddma_running) {
        mutex_unlock(&dma->mux);        
        return EINVAL;
    }

    /* activate dummyblock, otherwise blocked DMA will block all
       other communication too */
    SPIN_LOCK_IRQSAVE(dma->spin, flags);
    if (dma->blstat==ddmabl_blocked) {
        struct demand_dma_block *block;
        dma->writing_block=-2;
        block=&dma->dummyblock;
        block->status=dmablock_dma;
        start_dma(sc, block);
        pERROR(sc, "dma reactivated with dummyblock");
    }
    dma->blstat=ddmabl_aborting;
    SPIN_UNLOCK_IRQRESTORE(dma->spin, flags);

    /* stop sending data and switch both LEDs on */
    /* no error handling; timeout expected */
    _writereg32(sc, 0x808, 0x18);

    sis1100_disable_irq(sc, plxirq_dma0, 0);
    sc->plxirq_dma0_hook=0;

    /* enable DMA EOT# */
    plxwritereg(sc, DMAMODE0, plxreadreg(sc, DMAMODE0)|(1<<14));
    if (!(plxreadreg(sc, DMACSR0_DMACSR1)&0x10)) {
        int c=0;
        sis1100writereg(sc, sr, sr_abort_dma);
        while (!(plxreadreg(sc, DMACSR0_DMACSR1)&0x10) && (c++<1000000)) {}
        if (!(plxreadreg(sc, DMACSR0_DMACSR1)&0x10)) {
	    pERROR(sc, "dma_stop: DMA NOT STOPPED");
            mutex_unlock(&dma->mux);        
	    return EIO;
        }
    }
    /* disable remote access */
    sis1100writereg(sc, cr, cr_ready<<16);

    sis1100_flush_fifo(sc, 1);

    dma->status=ddma_ready;
    mutex_unlock(&dma->mux);

    return 0;
}

#if 0
int
sis1100_ddma_wait(struct sis1100_softc *sc, struct sis1100_fdata* fd,
        unsigned int* block)
{
    struct demand_dma* dma=&sc->demand_dma;
    int res;

#if 0
    pERROR(sc, "ddma_wait");
#endif

    mutex_lock(&dma->mux);

    /* are we the owner and DMA is running? */
    if ((dma->status!=dma_running) || (dma->owner!=fd)) {
        mutex_unlock(&dma->mux);
        return EINVAL;
    }

    sc->got_irqs=0;

    if ((plxreadreg(sc, DMACSR0_DMACSR1)&0x10)) {
        goto finished;
    }

#ifdef __NetBSD__
    while (!(res||(sc->got_irqs&(got_dma0)))) {
        res = tsleep(&sc->local_wait, PCATCH, "dma_wait", 10*hz);
    }
#else
    res=wait_event_interruptible (
        sc->local_wait,
        (sc->got_irqs & got_dma0)
    );
#endif
    if (res) {
        mutex_unlock(&dma->mux);
        return EINTR;
    }

finished:

    sis1100_disable_irq(sc, plxirq_dma0, 0);

    /*
    {
        size_t _bytes;
        _bytes=sis1100readreg(sc, byte_counter);
        pDEBUG(sc, "dma_wait: byte_count=%u", _bytes);
        if (bytes)
            *bytes=_bytes;
    }
    */

    dma->status=dma_ready;
    mutex_unlock(&dma->mux);

    return 0;
}
#endif

int
sis1100_ddma_mark(struct sis1100_softc *sc, struct sis1100_fdata* fd,
        unsigned int* idx)
{
    DECLARE_SPINLOCKFLAGS(flags)
    struct demand_dma* dma=&sc->demand_dma;
    struct demand_dma_block *block;

    /* are we the owner and DMA is running? */
    if (dma->owner!=fd) {
        pERROR(sc, "ddma_mark: not owner");
        return EPERM;
    }
    if (dma->status!=ddma_running) {
        pERROR(sc, "ddma_mark: not running");
        return ESRCH;
    }

    if (*idx>=dma->numblocks) {
        pERROR(sc, "ddma_mark: block %d is invalid", *idx);
        return EINVAL;
    }

    block=dma->block+*idx;

    /* sanity check */
    if (block->status!=(dmablock_full|dmablock_synced)) {
        pERROR(sc, "Xddma_mark: block %d has status %d", *idx, block->status);
        pERROR(sc, "Yddma_mark: status should be %d", dmablock_full|dmablock_synced);
        return EINVAL;
    }
    if (*idx!=dma->reading_block) {
        pERROR(sc, "ddma_mark: block %d is not the 'reading block' (%d)",
                *idx, dma->reading_block);
        return EINVAL;
    }

    /* now we know that we have the correct block and can do the real work */

    block->status=dmablock_free;
    dma->reading_block=DMA_NEXT(dma->reading_block);

    /* check whether we have to restart DMA
       because we change dma->is_blocked in competition to plxirq_dma0_hook
       we need the spinlock */
    SPIN_LOCK_IRQSAVE(dma->spin, flags);
    if (dma->blstat==ddmabl_blocked) {
        /* dma->writing_block should already point to our block */
        if (dma->writing_block!=*idx) {
            pERROR(sc, "ddma_mark: block %d is not the 'writing block' (%d)",
                *idx, dma->writing_block);
            return EINVAL;
        }
#if 0
        pERROR(sc, "dma is running again");
#endif
        dma->blstat=ddmabl_running;
        block->status=dmablock_dma;
        start_dma(sc, block);
    }
    SPIN_UNLOCK_IRQRESTORE(dma->spin, flags);

    /* set pending_irqs again if the next block is ready for reading */
    if (dma->block[dma->reading_block].status&dmablock_full) {
        SPIN_LOCK_IRQSAVE(sc->handlercommand.lock, flags);
        sc->pending_irqs|=ZELLVD_DDMA_IRQ;
        SPIN_UNLOCK_IRQRESTORE(sc->handlercommand.lock, flags);
        wake_up_process(sc->handler);
    }

    return 0;
}

int
sis1100_ddma_map(struct sis1100_softc *sc, struct sis1100_fdata* fd,
    struct sis1100_ddma_map* map)
{
    struct demand_dma* dma;
    int res, i;

    /* we are called from 'release' with map==0 */
    if ((map!=0) && (map->num!=0) && (map->size!=0)) {
        /*if (map->num<2)
            return EINVAL;*/

        if (map->size&(NBPG-1)) {
            pINFO(sc, "ddma_map: size not multiple of page size");
            return EINVAL;
        }

        if ((unsigned long)map->addr&(NBPG-1)) {
            pINFO(sc, "ddma_map: addr not at page boundary");
            return EINVAL;
        }
    }

    dma=&sc->demand_dma;
    mutex_lock(&dma->mux);

    /* is an other process using DMA? */
    if ((dma->status!=ddma_invalid) && (dma->owner!=fd)){
        mutex_unlock(&dma->mux);
        return EPERM;
    }
    /* is DMA still active? */
    if (dma->status==ddma_running) {
        mutex_unlock(&dma->mux);
        return EALREADY;
    }

    /* if DMA is initialized we will free all structures first */
    if (dma->status==ddma_ready) {
        for (i=0; i<dma->numblocks; i++)
            sis1100_ddma_unmap_block(sc, dma->block+i);
        KFREE(dma->block);
        dma->block=0;
        sis1100_ddma_free_dummyblock(sc, &dma->dummyblock);
        dma->status=ddma_invalid;
    }

    /* if the user did not request a new mapping we are done here */
    if ((map==0) || (map->num==0) || (map->size==0)) {
        mutex_unlock(&dma->mux);
        return 0;
    }

    pINFO(sc, "dma_map: size=%lld addr=%p num=%d",
        (unsigned long long)map->size, map->addr, map->num);

    dma->block=KMALLOC(map->num*sizeof(struct demand_dma_block));
    if (dma->block==0) {
        mutex_unlock(&dma->mux);
        return ENOMEM;
    }

    dma->owner=fd;
    dma->size=map->size;
    dma->numblocks=map->num;
    dma->uaddr=map->addr;
    for (i=0; i<dma->numblocks; i++)
        sis1100_ddma_zero(dma->block+i);

    for (i=0; i<dma->numblocks; i++) {
        struct demand_dma_block* block=dma->block+i;

        block->uaddr=dma->uaddr+i*dma->size;
        block->size=dma->size;
#ifdef __NetBSD__
        block->p=fd->p;
#endif
        /*pINFO(sc, "dma_map: try block %d", i);*/
        res=sis1100_ddma_map_block(sc, block);
        if (res) {
            int j;
            pINFO(sc, "dma_map: block %d: res=%d", i, res);
            for (j=0; j<dma->numblocks; j++)
                sis1100_ddma_unmap_block(sc, dma->block+j);
            KFREE(dma->block);
            dma->block=0;
            mutex_unlock(&dma->mux);
            return res;
        }
    }
    for (i=0; i<dma->numblocks; i++) {
        pERROR(sc, "dmadpr0[%d]=%08x", i, dma->block[i].dmadpr0);
    }

    if ((res=sis1100_ddma_alloc_dummyblock(sc, &dma->dummyblock))!=0)
        /* XXX free all other resources! */
        return res;

    dma->status=ddma_ready;
    mutex_unlock(&dma->mux);
    return 0;
}

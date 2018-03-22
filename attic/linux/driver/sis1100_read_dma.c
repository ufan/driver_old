/* $ZEL: sis1100_read_dma.c,v 1.14 2003/01/09 12:13:19 wuestner Exp $ */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/wrapper.h>
#include <linux/pci.h>
#include <linux/iobuf.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>

#include <dev/pci/sis1100_sc.h>

static ssize_t
_sis1100_read_dma(
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
    struct SIS1100_softc* sc=fd->sc;
    int res, i, sgcount, aborted=0;
    u_int32_t head, dmamode;
    sigset_t oldset;
#ifdef USE_SGL
    int nr_pages;
#else
    struct kiobuf* iobuf=sc->iobuf;
    int err, offs;
#endif
    /*printk(KERN_ERR "r addr=%d size=%d fifo=%d count=%ld data=%p\n",
                addr, size, fifo_mode, count, data);*/
    if (count>DMA_MAX) count=DMA_MAX;
    /*
    printk(KERN_ERR "DMA_MAX=%ld count=%ld SGL_SIZE=%d\n", DMA_MAX, count, SGL_SIZE);
    */
    {
        u_int32_t val;
        val=plxreadreg(sc, DMACSR0_DMACSR1);
        if (!(val&0x10)) {
            printk(KERN_CRIT "sis1100_read_dma: DMACSR0=%04x\n", val);
            printk(KERN_CRIT "sis1100_read_dma: old DMA still active.\n");
            return -EIO;
        }
    }

#ifdef USE_SGL
    nr_pages=sgl_map_user_pages(sc->sglist, SGL_SIZE, data, count, READ);
    /*printk(KERN_ERR "R nr_pages=%d\n", nr_pages);*/
    if (nr_pages<0) {
        printk(KERN_INFO "SIS1100[%d] sgl_map_user_pages failed\n", sc->unit);
        return nr_pages;
    }
    sgcount=pci_map_sg(sc->pcidev, sc->sglist, nr_pages,
        PCI_DMA_FROMDEVICE/*|0xf000*/);
    /*printk(KERN_ERR "R sgcount=%d\n", sgcount);*/
    if (!sgcount) {
        printk(KERN_ERR "SIS1100[%d] read_dma: pci_map_sg failed\n", sc->unit);
        sgl_unmap_user_pages(sc->sglist, nr_pages, 0);
        return -EIO;
    }
#else
    err=map_user_kiobuf(READ, iobuf, (unsigned long)data, count);
    if (err) {
        printk(KERN_INFO "SIS1100[%d] map_user_kiobuf failed\n", sc->unit);
        return err;
    }
    offs=iobuf->offset;
    for (i=0; i<iobuf->nr_pages-1; i++) {
        sc->sglist[i].address=0;
        sc->sglist[i].page=iobuf->maplist[i];
        sc->sglist[i].offset=offs;
        sc->sglist[i].length=PAGE_SIZE-offs;
        sc->sglist[i].dma_length=0;
        offs=0;
    }
    sc->sglist[i].address=0;
    sc->sglist[i].page=iobuf->maplist[i];
    sc->sglist[i].offset=offs;
    sc->sglist[i].length=iobuf->length-i*PAGE_SIZE+iobuf->offset-offs;
    sc->sglist[i].dma_length=0;
    sgcount=pci_map_sg(sc->pcidev, sc->sglist, iobuf->nr_pages,
            PCI_DMA_FROMDEVICE);
    if (!sgcount) {
        printk(KERN_ERR "SIS1100[%d] read_dma: pci_map_sg failed\n",
            sc->unit);
        unmap_kiobuf(iobuf);
        return -EIO;
    }
#endif

    dmamode=0x43|(1<<7)|(1<<8)|(1<<10)|(1<<11)|(1<<12)|(1<<14)|(1<<17);
    if (sgcount>1) { /* use scatter/gather mode */
        struct plx9054_dmadesc* desclist=
            (struct plx9054_dmadesc*)sc->descbuf.cpu_addr;
        struct scatterlist* sgl=sc->sglist;
        dma_addr_t next_handle=sc->descbuf.dma_handle;
        dmamode|=1<<9;
        for (i=sgcount; i; i--) {
            next_handle+=sizeof(struct plx9054_dmadesc);
            desclist->pcistart=cpu_to_le32(sg_dma_address(sgl));
            desclist->localstart=cpu_to_le32(0);
            desclist->size=cpu_to_le32(sg_dma_len(sgl));
            desclist->next=cpu_to_le32(next_handle|9);
            desclist++; sgl++;
        }
        desclist[-1].next|=2;
        plxwritereg(sc, DMADPR0, sc->descbuf.dma_handle|1);
    } else { /* only one page --> use block mode */
        plxwritereg(sc, DMAPADR0, sg_dma_address(sc->sglist));
        plxwritereg(sc, DMALADR0, 0);
        plxwritereg(sc, DMASIZ0, sg_dma_len(sc->sglist));
        plxwritereg(sc, DMADPR0, 8);
    }

/* prepare PLX */
    plxwritereg(sc, DMACSR0_DMACSR1, 1<<3); /* clear irq */
    plxwritereg(sc, DMAMODE0, dmamode);

/* prepare add on logic */
    /* 4 Byte, local space 2, BT, EOT, start with t_adl */
    head=0x0f80A002|(space&0x3f)<<16;
    if (am>=0) {
        head|=0x800;
        sis1100writereg(sc, t_am, am);
    }
    if (fifo_mode) head|=0x4000;
    sis1100writereg(sc, t_hdr, head);
    wmb();
    sis1100writereg(sc, t_dal, count);

    sis1100writereg(sc, d0_bc, 0);
    sis1100writereg(sc, d0_bc_buf, 0);

    sis1100writereg(sc, p_balance, 0);

/* block signals */
#if LINUX_VERSION_CODE < 0x20500
    spin_lock_irq(&current->sigmask_lock);
#else
    spin_lock_irq(&current->sig->siglock);
#endif
    oldset = current->blocked;
    sigfillset(&current->blocked);
    sigdelset(&current->blocked, SIGKILL);
    /* dangerous, should be removed later */
    /*if (!sigismember(&oldset, SIGINT)) sigdelset(&current->blocked, SIGINT);*/
#if LINUX_VERSION_CODE < 0x20500
    recalc_sigpending(current);
    spin_unlock_irq(&current->sigmask_lock);
#else
    recalc_sigpending();
    spin_unlock_irq(&current->sig->siglock);
#endif

/* enable dma */
    plxwritereg(sc, DMACSR0_DMACSR1, 3);

/* enable irq */
    sc->got_irqs=0;
    sis1100_enable_irq(sc, plxirq_dma0, irq_synch_chg|irq_s_xoff|
        irq_prot_l_err);

/* start transfer */
    mb();
    sis1100writereg(sc, t_adl, addr);
    wmb();

/* wait for dma */
    res=wait_event_interruptible(
	sc->local_wait,
	(sc->got_irqs & (got_dma0|got_xoff|got_sync|got_l_err))
	);
    if (sc->got_irqs&(got_dma0|got_l_err)) { /* transfer complete or error */
        count=sis1100readreg(sc, d0_bc);
        if (!(sc->got_irqs&got_dma0)) {
            u_int32_t val;
            val=plxreadreg(sc, DMACSR0_DMACSR1);
            printk(KERN_CRIT "SIS1100[%d] read_dma/abort: DMACSR0=0x%x\n",
                    sc->unit, val);
            if (!(val&0x10)) { /* DMA not stopped yet; abort it */
                sis1100writereg(sc, sr, sr_abort_dma);
            }
        }
    } else /*(res||(sc->got_irqs&(got_sync|got_xoff)))*/ { /* fatal */
        aborted=0x300;
        if (res) {
            printk(KERN_WARNING "SIS1100[%d] read_dma: interrupted\n", sc->unit);
            aborted|=1;
        }
        if (sc->got_irqs&got_sync) {
            printk(KERN_WARNING "SIS1100[%d] read_dma: synchronisation lost\n",
                    sc->unit);
            aborted|=2;
        }
        if (sc->got_irqs&got_xoff) {
            printk(KERN_CRIT "SIS1100[%d] read_dma: got xoff (irqs=0x%04x)\n",
                    sc->unit, sc->got_irqs);
            aborted|=4;
        }
        if (aborted==300) {
            printk(KERN_CRIT "SIS1100[%d] read_dma: got_irqs=0x%x\n",
                    sc->unit, sc->got_irqs);
        }
        sis1100writereg(sc, sr, sr_abort_dma);
    }

    sis1100_disable_irq(sc, plxirq_dma0, irq_s_xoff|irq_prot_end);
    plxwritereg(sc, DMACSR0_DMACSR1, 0);

#if LINUX_VERSION_CODE < 0x20500
    spin_lock_irq(&current->sigmask_lock);
#else
    spin_lock_irq(&current->sig->siglock);
#endif
    current->blocked = oldset;
#if LINUX_VERSION_CODE < 0x20500
    recalc_sigpending(current);
    spin_unlock_irq(&current->sigmask_lock);
#else
    recalc_sigpending();
    spin_unlock_irq(&current->sig->siglock);
#endif

    *prot_error=sis1100readreg(sc, prot_error);

    /*if (aborted) sis1100_dump_glink_status(sc, "after abort", 1);*/
    if (aborted) sis1100_flush_fifo(sc, "after abort", 0);
    if (aborted) *prot_error=aborted;
    /*if ((*prot_error!=0) && ((*prot_error&0x200)!=0x200)) count=-EIO;*/

#ifdef USE_SGL
    pci_unmap_sg(sc->pcidev, sc->sglist, nr_pages, PCI_DMA_FROMDEVICE);
    sgl_unmap_user_pages(sc->sglist, nr_pages, 1);
#else
    pci_unmap_sg(sc->pcidev, sc->sglist, iobuf->nr_pages, PCI_DMA_FROMDEVICE);
    unmap_kiobuf(iobuf);
#endif

    return count;
}

ssize_t
sis1100_read_dma(
    struct SIS1100_fdata* fd,
    u_int32_t addr,           /* VME or SDRAM address */
    int32_t am,               /* address modifier, not used if <0 */
    u_int32_t size,           /* datasize; should be 4 */
    int space,                /* remote space (1,2: VME; 6: SDRAM) */
    int fifo_mode,
    size_t count,             /* bytes to be transferred */
    u_int8_t* data,           /* destination (user virtual address) */
    int* prot_err
    )
{
    struct SIS1100_softc* sc=fd->sc;
    ssize_t res=1;
    size_t completed=0;

    *prot_err=0;

    if (!count) return 0;

    down(&sc->sem_hw);
    while (count && (res>0) && (*prot_err==0)) {
        res=_sis1100_read_dma(fd, addr, am, size, space, fifo_mode,
                count, data, prot_err);

        if (res>0) {
            if (!fifo_mode) addr+=res;
            data+=res;
            completed+=res;
            count-=res;
        }
    }
    up(&sc->sem_hw);

    if (completed)
        return completed;
    else
        return res;
}

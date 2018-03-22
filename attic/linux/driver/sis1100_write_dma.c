/* $ZEL: sis1100_write_dma.c,v 1.15 2003/01/09 12:13:20 wuestner Exp $ */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/wrapper.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <linux/iobuf.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>

#include <dev/pci/sis1100_sc.h>

static ssize_t
_sis1100_write_dma(
    struct SIS1100_fdata* fd,
    u_int32_t addr,           /* VME or SDRAM address */
    int32_t am,               /* address modifier, not used if <0 */
    u_int32_t size,           /* datasize must be 4 for DMA but is not checked*/
    int space,                /* remote space (1,2: VME; 6: SDRAM) */
    int fifo_mode,
    int count,                /* bytes to be transferred */
    const u_int8_t* data,     /* source (user virtual address) */
    int* prot_error
    )
{
    struct SIS1100_softc* sc=fd->sc;
    int res, i, sgcount, aborted=0;
    u_int32_t head, tmp, dmamode;
    sigset_t oldset;
#ifdef USE_SGL
    int nr_pages;
#else
    struct kiobuf* iobuf=sc->iobuf;
    int err, offs;
#endif
    /*printk(KERN_ERR "w addr=%d size=%d fifo=%d count=%d data=%p\n",
                addr, size, fifo_mode, count, data);*/
    if (count>DMA_MAX) count=DMA_MAX;

    if ((addr^(addr+count))&0x80000000U) count=0x80000000U-addr;

    {
        u_int32_t val;
        val=plxreadreg(sc, DMACSR0_DMACSR1);
        if (!(val&0x10)) {
            printk(KERN_CRIT "sis1100_write_dma: DMACSR0=%04x\n", val);
            printk(KERN_CRIT "sis1100_write_dma: old DMA still active.\n");
            return -EIO;
        }
    }

#ifdef USE_SGL
    nr_pages=sgl_map_user_pages(sc->sglist, SGL_SIZE, data, count, WRITE);
    /*printk(KERN_ERR "W nr_pages=%d\n", nr_pages);*/
    if (nr_pages<0) {
        printk(KERN_INFO "SIS1100[%d] sgl_map_user_pages failed\n", sc->unit);
        return nr_pages;
    }
/*dump_sgl(sc->sglist, nr_pages);*/
    sgcount=pci_map_sg(sc->pcidev, sc->sglist, nr_pages,
        PCI_DMA_TODEVICE|0xf000);
    /*printk(KERN_ERR "W sgcount=%d\n", sgcount);*/
    if (!sgcount) {
        printk(KERN_ERR "SIS1100[%d] write_dma: pci_map_sg failed\n",
            sc->unit);
        sgl_unmap_user_pages(sc->sglist, nr_pages, 0);
        return -EIO;
    }
#else
    printk(KERN_ERR "DON'T USE_SGL\n");
    err=map_user_kiobuf(WRITE, iobuf, (unsigned long)data, count);
    printk(KERN_ERR "err=%d\n", err);
    if (err) {
        printk(KERN_INFO "map_user_kiobuf failed\n");
        return err;
    }
    printk(KERN_ERR "nr_pages=%d\n", iobuf->nr_pages);

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
            PCI_DMA_TODEVICE);
    printk(KERN_ERR "sgcount=%d\n", sgcount);
    if (!sgcount) {
        printk(KERN_ERR "SIS1100[%d] read_dma: pci_map_sg failed\n",
            sc->unit);
        unmap_kiobuf(iobuf);
        return -EIO;
    }
#endif
    dmamode=0x43|(1<<7)|(1<<8)|(1<<10)|(1<<14)|(1<<17);
    if (sgcount>1) { /* use scatter/gather mode */
        struct plx9054_dmadesc* desclist=
            (struct plx9054_dmadesc*)sc->descbuf.cpu_addr;
        struct scatterlist* sgl=sc->sglist;
        u_int32_t local=addr&0x7fffffffU;
        dma_addr_t next_handle=sc->descbuf.dma_handle;
        dmamode|=1<<9;
        for (i=sgcount; i; i--) {
            /*printk(KERN_ERR "sgl(%d-%d)=%p\n", sgcount, i, sgl);*/
            next_handle+=sizeof(struct plx9054_dmadesc);
            desclist->pcistart=cpu_to_le32(sg_dma_address(sgl));
            desclist->localstart=cpu_to_le32(local);
            desclist->size=cpu_to_le32(sg_dma_len(sgl));
            desclist->next=cpu_to_le32(next_handle|1);
            if (!fifo_mode) local+=sg_dma_len(sgl);
            desclist++; sgl++;
        }
        desclist[-1].next|=2;
        plxwritereg(sc, DMADPR0, sc->descbuf.dma_handle|1);
    } else { /* only one page --> use block mode */
        /*printk(KERN_ERR "dma_address=0x%08x\n", sg_dma_address(sc->sglist));*/
        plxwritereg(sc, DMAPADR0, sg_dma_address(sc->sglist));
        plxwritereg(sc, DMALADR0, addr&0x7fffffffU);
        plxwritereg(sc, DMASIZ0, sg_dma_len(sc->sglist));
        plxwritereg(sc, DMADPR0, 0);
    }

/* prepare PLX */
    plxwritereg(sc, DMACSR0_DMACSR1, 1<<3); /* clear irq */
    plxwritereg(sc, DMAMODE0, dmamode);

/* prepare add on logic */
    /* 4 Byte, local space 2, BT, EOT, start with t_adl */
    head=0x0f80A402|(space&0x3f)<<16;
    if (am>=0) {
        head|=0x800;
        sis1100writereg(sc, d_am, am);
    }
    if (fifo_mode) head|=0x4000;
    sis1100writereg(sc, d_hdr, head);
    wmb();
    sis1100writereg(sc, d_adl, addr); /* only bit 31 is valid */

    sis1100writereg(sc, d_bc, count);

    sis1100writereg(sc, p_balance, 0);

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

/* enable irq */
    /* irq_synch_chg and irq_prot_l_err should always be enabled */
    sis1100_enable_irq(sc, 0,
        irq_prot_l_err|irq_synch_chg|irq_s_xoff|irq_prot_end);

/* start dma */
    sc->got_irqs=0;
    mb();
    plxwritereg(sc, DMACSR0_DMACSR1, 3);

/* wait for confirmation */
    res=wait_event_interruptible(
	sc->local_wait,
	(sc->got_irqs & (got_end|got_xoff|got_sync|got_l_err))
	);

    if (sc->got_irqs&got_l_err) {
        printk(KERN_CRIT "SIS1100: irq_prot_l_err in write_dma, irqs=0x%04x\n",
            sc->got_irqs);
    }
    if (res|(sc->got_irqs&(got_sync|got_xoff))) {
        aborted=0x300;
        if (res) {
            printk(KERN_INFO "SIS1100[%d] write_dma: interrupted\n", sc->unit);
            aborted|=1;
        }
        if (sc->got_irqs&got_sync) {
            printk(KERN_WARNING "SIS1100[%d] write_dma: synchronisation lost\n",
                    sc->unit);
            aborted|=2;
        }
        if (sc->got_irqs&got_xoff) {
            printk(KERN_CRIT "SIS1100[%d] write_dma: got xoff (irqs=0x%04x)\n",
                    sc->unit, sc->got_irqs);
            aborted|=4;
        }
    }

    sis1100_disable_irq(sc, 0, irq_s_xoff|irq_prot_end);

#if LINUX_VERSION_CODE < 0x20500
    spin_lock_irq(&current->sigmask_lock);
    current->blocked = oldset;
    recalc_sigpending(current);
    spin_unlock_irq(&current->sigmask_lock);
#else
    spin_lock_irq(&current->sig->siglock);
    current->blocked = oldset;
    recalc_sigpending();
    spin_unlock_irq(&current->sig->siglock);
#endif

    *prot_error=sis1100readreg(sc, prot_error);

    if (aborted) {
        *prot_error=aborted;
        count=-EIO;
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
                 printk(KERN_WARNING "SIS1100[%d] write_dma: "
                    "read count after error: prot_error=0x%03x\n",
                    sc->unit, tmp);
                count=-EIO;
            } else {
                count=sis1100readreg(sc, tc_dal);
            }
        } else {
            count=-EIO;
        }
    }

    if (aborted) sis1100_dump_glink_status(sc, "after abort", 1);

#ifdef USE_SGL
    pci_unmap_sg(sc->pcidev, sc->sglist, nr_pages, PCI_DMA_TODEVICE);
    sgl_unmap_user_pages(sc->sglist, nr_pages, 1);
#else
    pci_unmap_sg(sc->pcidev, sc->sglist, iobuf->nr_pages, PCI_DMA_TODEVICE);
    unmap_kiobuf(iobuf);
#endif

    return count;
}

ssize_t
sis1100_write_dma(
    struct SIS1100_fdata* fd,
    u_int32_t addr,           /* VME or SDRAM address */
    int32_t am,               /* address modifier, not used if <0 */
    u_int32_t size,           /* datasize must be 4 for DMA but is not checked*/
    int space,                /* remote space (1,2: VME; 6: SDRAM) */
    int fifo_mode,
    size_t count,             /* bytes to be transferred */
    const u_int8_t* data,     /* source (user virtual address) */
    int* prot_err
    )
{
    struct SIS1100_softc* sc=fd->sc;
    ssize_t res=1, completed=0;

    *prot_err=0;

    if (!count) return 0;

    if (!access_ok(VERIFY_READ, data, count)) return -EFAULT;

    down(&sc->sem_hw);
    while (count && (res>0) && (*prot_err==0)) {
        res=_sis1100_write_dma(fd, addr, am, size, space, fifo_mode,
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

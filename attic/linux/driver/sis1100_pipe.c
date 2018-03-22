/* $ZEL: sis1100_pipe.c,v 1.9 2003/01/09 12:13:18 wuestner Exp $ */

#include "Copyright"

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/wrapper.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include <dev/pci/sis1100_sc.h>

int
sis1100_read_pipe(struct SIS1100_softc* sc, struct sis1100_pipe* control)
{
    struct sis1100_pipelist* list;
    struct SIS1100_dmabuf dmabuf;
    int i, error, balance=0, res=0;

    list=kmalloc(control->num*sizeof(struct sis1100_pipelist), GFP_USER);
    if (!list) return -ENOMEM;

    if (copy_from_user(list, control->list,
    	    control->num*sizeof(struct sis1100_pipelist))) {
    	kfree(list);
    	return -EFAULT;
    }

    /* how many bytes do we have to read? */
    dmabuf.size=0;
    for (i=0; i<control->num; i++) {
        if (!list[i].head&0x400) dmabuf.size+=sizeof(u_int32_t);
    }

    if (dmabuf.size) {
        dmabuf.cpu_addr=pci_alloc_consistent(sc->pcidev,
    	    dmabuf.size, &dmabuf.dma_handle);
        if (!dmabuf.cpu_addr) {
    	    kfree(list);
    	    return -ENOMEM;
        }
    } else {
        dmabuf.size=0;
        dmabuf.dma_handle=0;
    }

    down(&sc->sem_hw);
    sis1100writereg(sc, rd_pipe_buf, dmabuf.dma_handle);
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

    res=wait_event_interruptible(
            sc->local_wait,
            ((balance=sis1100readreg(sc, p_balance))==0)
        );

    sis1100_disable_irq(sc, 0, irq_prot_end);

    error=sis1100readreg(sc, prot_error);
    if (!balance) sis1100writereg(sc, p_balance, 0);

    if (error||res) {
    	/*printk(KERN_INFO
    	    "sis1100_read_pipe: error=0x%0x, res=%d\n", error, res);
	dump_glink_status(sc, "after pipe", 1);*/
    	sis1100_flush_fifo(sc, "pipe", 1);
    }
    {
    	int a, l;
    	a=sis1100readreg(sc, rd_pipe_buf);
    	l=sis1100readreg(sc, rd_pipe_blen);
    	if (!error && (l!=0))
            printk(KERN_WARNING "sis1100_read_pipe: rd_pipe_blen=%d\n", l);
    }
    up(&sc->sem_hw);
    control->error=error;

    kfree(list);
    if (copy_to_user(control->data, dmabuf.cpu_addr, dmabuf.size)) {
    	mem_map_unreserve(virt_to_page(dmabuf.cpu_addr));
    	pci_free_consistent(sc->pcidev, dmabuf.size,
    	    dmabuf.cpu_addr, dmabuf.dma_handle);
    	res=-EFAULT;
    }
    if (dmabuf.size)
        pci_free_consistent(sc->pcidev, dmabuf.size, dmabuf.cpu_addr,
            dmabuf.dma_handle);

    return res;
}

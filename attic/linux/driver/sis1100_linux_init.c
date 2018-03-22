/* $ZEL: sis1100_linux_init.c,v 1.19 2003/01/09 12:13:17 wuestner Exp $ */

/*
 * Copyright (c) 2001/2002
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
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

#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/iobuf.h>
#include <linux/wrapper.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include <dev/pci/sis1100_sc.h>

#ifdef USE_DEVFS
devfs_handle_t dir;
#else
int SIS1100_major = -1;
#endif

struct pci_device_id SIS1100_table[] __devinitdata={
    {
    0x1796, 0x0001,
    PCI_ANY_ID, PCI_ANY_ID,
    0, 0,
    0
    },
    { 0 }
};

MODULE_AUTHOR("Peter Wuestner <P.Wuestner@fz-juelich.de>");
MODULE_DESCRIPTION("SIS1100 PCI-VME link/interface (http://zelweb.zel.kfa-juelich.de/projects/gigalink/)");
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif
MODULE_SUPPORTED_DEVICE("sis1100/sis3100/sis5100; http://www.struck.de/pcivme.htm");

struct SIS1100_softc *SIS1100_devdata[sis1100_MAXCARDS];

struct file_operations SIS1100_fops = {
	.owner   = THIS_MODULE,
	.open    = sis1100_open,
	.release = sis1100_release,
	.ioctl   = sis1100_ioctl,
	.llseek  = sis1100_llseek,
	.read    = sis1100_read,
	.write   = sis1100_write,
	.mmap    = sis1100_mmap,
};

void __init SIS1100_print_info(void)
{
    printk(KERN_INFO "SIS1100 driver V2.0 (c) 28.11.2002 FZ Juelich\n");
}

int __init
SIS1100_linux_init(struct pci_dev *dev)
{
	struct SIS1100_softc *sc;
	int i, idx, res;
	u_long pmembase;
#ifdef USE_DEVFS
        char devname[100];
#endif

#if LINUX_VERSION_CODE < 0x20500
	printk(KERN_INFO "SIS1100: found %s at %s\n", dev->name, dev->slot_name);
#else
	printk(KERN_INFO "SIS1100: found %s at %s\n", dev->dev.name, dev->slot_name);
#endif
/*
        printk(KERN_INFO "vendor=0x%04x\n", dev->vendor);
        printk(KERN_INFO "device=0x%04x\n", dev->device);
        printk(KERN_INFO "sub_vendor=0x%04x\n", dev->subsystem_vendor);
        printk(KERN_INFO "sub_device=0x%04x\n", dev->subsystem_device);
        printk(KERN_INFO "class=%d\n", dev->class);
#if LINUX_VERSION_CODE < 0x20500
        printk(KERN_INFO "name=>%s<\n", dev->name);
#else
        printk(KERN_INFO "name=>%s<\n", dev->dev.name);
#endif
        printk(KERN_INFO "slot_name=>%s<\n", dev->slot_name);
*/
	for (idx = 0; idx < sis1100_MAXCARDS; idx++) {
		if (!SIS1100_devdata[idx]) break;
	}
	if (idx == sis1100_MAXCARDS) return -ENOMEM;

	sc = kmalloc(sizeof(struct SIS1100_softc), GFP_KERNEL);
	if (!sc) return -ENOMEM;
	SIS1100_devdata[idx] = sc;
	sc->unit = idx;

        for (i=0; i<=sis1100_MINORUTMASK; i++) sc->fdatalist[i]=0;

	init_waitqueue_head(&sc->handler_wait);
	init_waitqueue_head(&sc->local_wait);
	init_waitqueue_head(&sc->irq_wait);
	init_timer(&sc->link_up_timer);
        sc->link_up_timer.function=sis1100_synch_s_handler;
        sc->link_up_timer.data=(unsigned long)sc;

        init_MUTEX (&sc->sem_hw);
        init_MUTEX (&sc->sem_fdata_list);
        spin_lock_init(&sc->lock_intcsr);
        spin_lock_init(&sc->handlercommand.lock);
        sc->handlercommand.command=0;
        INIT_LIST_HEAD(&sc->fdata_list_head);
        init_completion(&sc->handler_completion);

        sc->handler_pid=kernel_thread(sis1100_irq_handler, sc, 0);
        if (sc->handler_pid<0) {
            printk(KERN_ERR "create sis1100_irq_handler: %d\n", sc->handler_pid);
            return sc->handler_pid;
        }

	pmembase = pci_resource_start(dev, 0);
	sc->plxmemlen = pci_resource_len(dev, 0);
	sc->plxmembase = ioremap(pmembase, sc->plxmemlen);
        printk(KERN_INFO "SIS1100: plxmembase=0x%08lx\n", pmembase);
        printk(KERN_INFO "mapped at %p (size=0x%x)\n", sc->plxmembase, sc->plxmemlen);
	if (!sc->plxmembase) {
            printk(KERN_ERR "SIS1100: can't map plx space\n");
	    return -ENOMEM;
        }
	pmembase = pci_resource_start(dev, 2);
	sc->reg_size = pci_resource_len(dev, 2);
	sc->reg_base = ioremap(pmembase, sc->reg_size);
        printk(KERN_INFO "SIS1100: reg_base=0x%08lx\n", pmembase);
        printk(KERN_INFO "mapped at %p (size=0x%x)\n", sc->reg_base, sc->reg_size);
	if (!sc->reg_base) {
            printk(KERN_ERR "SIS1100: can't map register space\n");
	    return -ENOMEM;
        }
	pmembase = pci_resource_start(dev, 3);
	sc->rem_size = pci_resource_len(dev, 3);
	sc->rem_base = ioremap(pmembase, sc->rem_size);
        printk(KERN_INFO "SIS1100: rem_base=0x%08lx\n", pmembase);
        printk(KERN_INFO "mapped at %p (size=0x%x)\n", sc->rem_base, sc->rem_size);
	if (!sc->rem_base) {
            printk(KERN_WARNING "SIS1100: can't map remote space\n");
	    printk(KERN_WARNING "SIS1100: mmap not available\n");
            sc->rem_size=0;
        }
	res = pci_request_regions(dev, "SIS1100");
	if (res)
		return (res);

	res = request_irq(dev->irq, SIS1100_intr, SA_SHIRQ, "SIS1100", sc);
	if (res)
		goto fehler;
        /*printk(KERN_INFO "SIS1100: irq %d installed\n", dev->irq);*/

	sc->pcidev = dev;
	pci_set_drvdata(dev, sc);

	pci_set_master(dev);

#if 0
        sc->no_dma=0;
        /*if (!pci_set_dma_mask(dev, 0xffffffffffffffff)) {
            printk(KERN_WARNING "SIS1100: 64bit DMA available (but not used yet).\n");
            sc->dma_dac=1;
        } else*/ if (!pci_set_dma_mask(dev, 0xffffffff)) {
            sc->dma_dac=0;
        } else {
            printk(KERN_WARNING "SIS1100: DMA not available.\n");
            sc->no_dma=1; /* not used yet */
        }
#endif

#ifndef USE_SGL
        if ((res=alloc_kiovec(1, &sc->iobuf))<0) {
                sc->iobuf=0;
                goto fehler;
        }
#endif
        sc->descbuf.size=SGL_SIZE*sizeof(struct plx9054_dmadesc);
        sc->descbuf.cpu_addr=pci_alloc_consistent(sc->pcidev,
    	        sc->descbuf.size, &sc->descbuf.dma_handle);
        if (!sc->descbuf.cpu_addr) {
                printk(KERN_ERR "SIS1100: pci_alloc_consistent failed\n");
                res=-ENOMEM;
                goto fehler;
        }
        /*mem_map_reserve(virt_to_page(sc->descbuf.cpu_addr));*/
        printk(KERN_INFO "SIS1100: descbuf.dma_handle=0x%08llx\n",
                (unsigned long long)sc->descbuf.dma_handle);
        /*
        printk(KERN_INFO "SIS1100: descbuf.cpu_addr=0x%08llx\n",
                (unsigned long long)sc->descbuf.cpu_addr);
        */
    	res=SIS1100_init(sc);
	if (res) {
	    goto fehler;
	}

#ifdef USE_DEVFS
        dir=devfs_mk_dir(NULL, "sis1100", 0);
        if (!dir) {
                printk(KERN_ERR "SIS1100 cannot create [devfs]/sis1100\n");
                return -EBUSY;
        }
        printk(KERN_INFO "SIS1100: %d", sc->unit);
        sc->devfs_dev=devfs_register(dir, devname, DEVFS_FL_AUTO_DEVNUM,
                0, 0, S_IFCHR|S_IRUGO|S_IWUGO, &SIS1100_fops,
                (void*)(0xffff0000|(0<<16)|sc->unit));
        printk(KERN_INFO "SIS1100: %dsd", sc->unit);
#endif

    	return 0;

    fehler:
	free_irq(dev->irq, sc);
#ifndef USE_SGL
        if (sc->iobuf) free_kiovec(1, &sc->iobuf);
        /*mem_map_unreserve(virt_to_page(sc->descbuf.cpu_addr));*/
#endif
        if (sc->descbuf.cpu_addr) {
                mem_map_unreserve(virt_to_page(sc->descbuf.cpu_addr));
                pci_free_consistent(sc->pcidev, sc->descbuf.size,
                        sc->descbuf.cpu_addr, sc->descbuf.dma_handle);
        }
	iounmap((void *)sc->plxmembase);
	iounmap((void *)sc->reg_base);
	iounmap((void *)sc->rem_base);
	SIS1100_devdata[sc->unit] = 0;

	kfree(sc);
	pci_release_regions(dev);
	pci_set_drvdata(dev, NULL);
    	return res;
}

void __exit
SIS1100_linux_done(struct pci_dev *dev)
{
	struct SIS1100_softc *sc;

	sc = pci_get_drvdata(dev);

#ifndef USE_SGL
        free_kiovec(1, &sc->iobuf);
        /*mem_map_unreserve(virt_to_page(sc->descbuf.cpu_addr));*/
#endif
        pci_free_consistent(sc->pcidev, sc->descbuf.size,
                sc->descbuf.cpu_addr, sc->descbuf.dma_handle);

#ifdef USE_DEVFS
        devfs_unregister(sc->devfs_dev);
        devfs_unregister(sc->devfs_dev_sd);
        devfs_unregister(sc->devfs_dev_sh);
#endif
       
	SIS1100_done(sc);
	free_irq(dev->irq, sc);
	del_timer_sync(&sc->link_up_timer);
	if (sc->handler_pid>=0) {
            unsigned long flags;
            spin_lock_irqsave(&sc->handlercommand.lock, flags);
            sc->handlercommand.command=handlercomm_die;
            spin_unlock_irqrestore(&sc->handlercommand.lock, flags);
            wake_up(&sc->handler_wait);
            wait_for_completion (&sc->handler_completion);
	}
	iounmap((void *)sc->plxmembase);
	iounmap((void *)sc->reg_base);
	iounmap((void *)sc->rem_base);
	SIS1100_devdata[sc->unit] = 0;

	kfree(sc);
	pci_release_regions(dev);
	pci_set_drvdata(dev, NULL);
}

#ifdef USE_DEVFS
int __init
SIS1100_linux_drvinit(void)
{
    return (0);
}
#else
int __init
SIS1100_linux_drvinit(void)
{
    SIS1100_major = register_chrdev(0, "SIS1100", &SIS1100_fops);
    printk(KERN_INFO "SIS1100 major=%d\n", SIS1100_major);
    if (SIS1100_major<0) {
        printk(KERN_INFO "SIS1100: unable to register device\n");
        return -EBUSY;
    }
    return (0);
}
#endif

#ifdef USE_DEVFS
void __exit
SIS1100_linux_drvdone(void)
{
    devfs_unregister(dir);
}
#else
void __exit
SIS1100_linux_drvdone(void)
{
    unregister_chrdev(SIS1100_major, "SIS1100");
}
#endif

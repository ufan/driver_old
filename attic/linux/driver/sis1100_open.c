/* $ZEL: sis1100_open.c,v 1.13 2003/01/09 12:13:18 wuestner Exp $ */

/*
 * Copyright (c) 2001
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/wrapper.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>

#include <dev/pci/sis1100_sc.h>

int
sis1100_open(struct inode *inode, struct file *file)
{
    struct SIS1100_softc* sc;
    struct SIS1100_fdata* fd;
    unsigned int _minor=minor(inode->i_rdev);
    unsigned int card=(_minor&sis1100_MINORCARDMASK)>>sis1100_MINORCARDSHIFT;
    unsigned int subdev=(_minor&sis1100_MINORTYPEMASK)>>sis1100_MINORTYPESHIFT;
    unsigned int idx=_minor&(sis1100_MINORUTMASK);

    if (card >= sis1100_MAXCARDS || !SIS1100_devdata[card]) {
        printk(KERN_INFO "sis1100 open: returning ENXIO\n");
        return -ENXIO; /*ENODEV*/
    }
    sc=SIS1100_devdata[card];

    if (sc->fdatalist[idx]) {
        /*
        printk(KERN_INFO "sis1100 open: fdatalist[%d]=%p\n", idx, sc->fdatalist[idx]);
        printk(KERN_INFO "sis1100 open: returning EBUSY\n");
        */
        return -EBUSY;
    }

    fd=kmalloc(sizeof(struct SIS1100_fdata), GFP_KERNEL);
    if (!fd) return -ENOMEM;
    fd->sc=sc;
    fd->subdev=subdev;
    file->private_data = fd;

    down(&sc->sem_fdata_list);
    list_add(&fd->list, &sc->fdata_list_head);
    up(&sc->sem_fdata_list);

    fd->fifo_mode=0;          /* can't be changed for sdram and sharc */
    fd->vmespace_am=9;        /* useless for sdram and sharc */
    fd->vmespace_datasize=4;  /* useless for sdram and sharc */
    fd->last_prot_err=0;
    fd->sig=0;
    fd->owned_irqs=0;         /* useless for sdram and sharc */
    fd->mindmalen_r=24;
    fd->mindmalen_w=400;
    fd->pid=current->pid;
    sc->fdatalist[idx]=fd;

    return 0;
}

int
sis1100_release(struct inode *inode, struct file *file)
{
        struct SIS1100_softc* sc=SIS1100SC(file);
        struct SIS1100_fdata* fd=SIS1100FD(file);
        unsigned int _minor=minor(inode->i_rdev);
        unsigned int idx=_minor&(sis1100_MINORUTMASK);

        if (fd->owned_irqs & SIS3100_IRQS) {
            switch (sc->remote_hw) {
            case sis1100_hw_vme:
                sis3100writeremreg(sc, vme_irq_sc,
                        (fd->owned_irqs & SIS3100_IRQS)<<16, 0);
                break;
            case sis1100_hw_camac:
                /*sis5100writeremreg(sc, vme_irq_sc,
                        (fd->owned_irqs & SIS3100_IRQS)<<16, 0);*/
                break;
            case sis1100_hw_pci: break; /* do nothing */
            case sis1100_hw_invalid: break; /* do nothing */
            }
        }
        down(&sc->sem_fdata_list);
        list_del(&fd->list);
        up(&sc->sem_fdata_list);

        sc->fdatalist[idx]=0;
        kfree(fd);
        return 0;
}

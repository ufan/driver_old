/* $ZEL: sis1100_mmap.c,v 1.5 2003/01/09 12:13:18 wuestner Exp $ */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/wrapper.h>
#include <linux/pci.h>
#include <asm/uaccess.h>

#include <dev/pci/sis1100_sc.h>
#define USE_PCI_MMAP

int
sis1100_mmap(struct file * file, struct vm_area_struct * vma)
{
    struct SIS1100_softc *sc = SIS1100SC(file);
    struct SIS1100_fdata *fd = SIS1100FD(file);
    int error;

    u_long pmembase=0;
    u_int32_t pmemsize=0;

    switch (fd->subdev) {
        case sis1100_subdev_remote:
            if (!sc->rem_size)
                    return -ENOTTY;
            pmembase=pci_resource_start(sc->pcidev, 3);
            pmemsize=sc->rem_size;
            break;
        case sis1100_subdev_ctrl:
            pmembase=pci_resource_start(sc->pcidev, 2);
            pmemsize=sc->reg_size;
            break;
        case sis1100_subdev_ram: /* nobreak */
        case sis1100_subdev_dsp:
            return -ENOTTY;
    }

    /*   offset in bytes           + size of mapping          */
    if ((vma->vm_pgoff<<PAGE_SHIFT)+(vma->vm_end-vma->vm_start)>pmemsize)
    	    return -EINVAL;

#ifdef USE_PCI_MMAP
    /* this does only work if pmembase is a multiple of PAGE_SIZE */
    vma->vm_pgoff+=pmembase>>PAGE_SHIFT;
    if ((error=pci_mmap_page_range(sc->pcidev, vma, pci_mmap_mem, 0))<0)
        return error;
#else
    vma->vm_flags |= VM_RESERVED;
    vma->vm_flags |= VM_IO;

    if ((error=remap_page_range(
#if LINUX_VERSION_CODE >= 0x20500
            vma,
#endif
            vma->vm_start, pmembase+(vma->vm_pgoff<<PAGE_SHIFT),
    	    vma->vm_end-vma->vm_start, vma->vm_page_prot))!=0)
    	return error;
#endif

    return 0;
}

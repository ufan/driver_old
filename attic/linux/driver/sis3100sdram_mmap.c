/* $ZEL: sis3100sdram_mmap.c,v 1.2 2002/08/12 10:36:41 wuestner Exp $ */

#include "Copyright"

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/wrapper.h>
#include <linux/pci.h>
#include <asm/uaccess.h>

#include <dev/pci/sis1100_sc.h>

int
sis3100sdram_mmap(struct file * file, struct vm_area_struct * vma)
{
/*
    struct SIS1100_softc* sc=SIS3100sdramSC(file);
    struct SIS3100sdram_fdata* fd=SIS3100sdramFD(file);
*/
    return -EINVAL;
}

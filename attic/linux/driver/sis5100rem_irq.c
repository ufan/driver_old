/* $ZEL: sis5100rem_irq.c,v 1.1 2003/01/09 12:16:06 wuestner Exp $ */

#include "Copyright"

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/wrapper.h>
#include <linux/pci.h>
#include <linux/list.h>
#include <asm/uaccess.h>

#include <dev/pci/sis1100_sc.h>

void
sis5100rem_irq_handler(struct SIS1100_softc* sc)
{
}

void
sis5100rem_enable_irqs(struct SIS1100_fdata* fd, u_int32_t mask)
{
}

void
sis5100rem_disable_irqs(struct SIS1100_fdata* fd, u_int32_t mask)
{
}

void
sis5100rem_irq_ack(struct SIS1100_softc* sc, int irqs)
{
}

/* $ZEL: sis5100rem_front_io.c,v 1.1 2003/01/09 12:16:06 wuestner Exp $ */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/pci.h>

#include <dev/pci/sis1100_sc.h>

void
sis5100rem_front_io(struct SIS1100_softc* sc, u_int32_t* data, int locked)
{
}

void
sis5100rem_front_pulse(struct SIS1100_softc* sc, u_int32_t* data, int locked)
{
}

void
sis5100rem_front_latch(struct SIS1100_softc* sc, u_int32_t* data, int locked)
{
}

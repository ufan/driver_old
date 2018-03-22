/* $ZEL: sis1100_device.c,v 1.3 2003/01/15 14:16:59 wuestner Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <vm/vm.h>
#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/pci/sis1100_var.h>
#include <dev/pci/sis1100_sc.h>

cdev_decl(sync);

struct cdevsw sis1100cdevsw = {
	sis1100_open,
	sis1100_close,
	sis1100_read,
	sis1100_write,
	sis1100_ioctl,
	(dev_type_stop((*))) enodev,
	0,
	(dev_type_poll((*))) enodev,
	sis1100_mmap,
	0
};

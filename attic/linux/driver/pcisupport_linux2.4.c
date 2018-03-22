/* $ZEL: pcisupport_linux2.4.c,v 1.4 2001/11/13 19:04:54 wuestner Exp $ */

/*
 * Copyright (c) 2001
 * 	Matthias Drochner.  All rights reserved.
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
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/pci.h>


#define __CONCAT(x,y)	x ## y
#define __STRING(x)	#x
#define DEVINITFUNC(mn) __CONCAT(mn, _linux_init)
#define DEVDONEFUNC(mn) __CONCAT(mn, _linux_done)
#define DRVINITFUNC(mn) __CONCAT(mn, _linux_drvinit)
#define DRVDONEFUNC(mn) __CONCAT(mn, _linux_drvdone)
#define PCITBLNAME(mn) __CONCAT(mn, _table)
#define MODULINFOFUNC(mn) __CONCAT(mn, _print_info)
#define __SS(s) __STRING(s)

int DEVINITFUNC(MODULENAME)(struct pci_dev *);
void DEVDONEFUNC(MODULENAME)(struct pci_dev *);
int DRVINITFUNC(MODULENAME)(void);
void DRVDONEFUNC(MODULENAME)(void);
void MODULINFOFUNC(MODULENAME)(void);

static int
device_init(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int res;

	res = pci_enable_device (pdev);
	if (res)
		return (res);
	return (DEVINITFUNC(MODULENAME)(pdev));
}

static void
device_done(struct pci_dev *pdev)
{

	DEVDONEFUNC(MODULENAME)(pdev);
}

extern struct pci_device_id PCITBLNAME(MODULENAME)[];

static struct pci_driver driver = {
	name:		__STRING(MODULENAME),
	id_table:	PCITBLNAME(MODULENAME),
	probe:		device_init,
	remove:		device_done,
};

static int __init
init_pcidrv_module(void)
{
    	int res;

    	MODULINFOFUNC(MODULENAME)();
	res = pci_module_init(&driver);
	if (res)
	    return (res);

        return (DRVINITFUNC(MODULENAME)());
}

static void __exit
cleanup_pcidrv_module(void)
{
    	printk(KERN_INFO __SS(MODULENAME) " exit\n");
	DRVDONEFUNC(MODULENAME)();
	pci_unregister_driver(&driver);
}

module_init(init_pcidrv_module);
module_exit(cleanup_pcidrv_module);

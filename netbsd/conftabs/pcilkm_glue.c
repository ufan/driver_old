/* $ZEL: pcilkm_glue.c,v 1.2 2003/09/09 11:09:07 wuestner Exp $ */

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/syscallargs.h>
#include <sys/conf.h>

#include <sys/lkm.h>

#include <sys/device.h>
#include "pcisupport.h"

extern struct cdevsw CDEVSW;

#define MODNAME(n) __STRING(n)
MOD_DEV(MODNAME(LKMNAME), LM_DT_CHAR, -1, &CDEVSW);

int pcilkmhdl(struct lkm_table *, int);

#define ENTRYNAME(n) __CONCAT(n, _lkmentry)
int ENTRYNAME(LKMNAME)(struct lkm_table *, int, int);

extern struct cfdata CFDATA;

int
pcilkmhdl(lkmtp, cmd)
	struct lkm_table *lkmtp;	
	int cmd;
{

	switch (cmd) {
	case LKM_E_LOAD:
		if (lkmexists(lkmtp))
			return (EEXIST);
		return (pcidev_load(&CFDATA));
	case LKM_E_UNLOAD:
		return (pcidev_unload(&CFDATA));
	case LKM_E_STAT:
		return (0);
	default:
		return (EINVAL);
	}
}

int
ENTRYNAME(LKMNAME)(lkmtp, cmd, ver)
	struct lkm_table *lkmtp;	
	int cmd;
	int ver;
{

	DISPATCH(lkmtp, cmd, ver, pcilkmhdl, pcilkmhdl, pcilkmhdl);
}

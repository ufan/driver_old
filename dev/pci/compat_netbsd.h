/* $ZEL: compat_netbsd.h,v 1.7 2007/09/11 21:33:34 wuestner Exp $ */

/*
 * Copyright (c) 2003-2004
 * 	Peter Wuestner.  All rights reserved.
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

#ifndef _compat_netbsd_h_
#define _compat_netbsd_h_

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/types.h>
#include <machine/db_machdep.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>

/*#define ofs(what, elem) ((off_t)&(((what *)0)->elem))*/
#define ofs(what, elem) ((unsigned long)&(((what *)0)->elem))

#define _plxreadreg_4(sc, offset) \
    bus_space_read_4(sc->plx_t, sc->plx_h, offset)
#define _plxwritereg_4(sc, offset, val) \
    bus_space_write_4(sc->plx_t, sc->plx_h, offset, val)

#define _plxreadreg_1(sc, offset) \
    bus_space_read_1(sc->plx_t, sc->plx_h, offset)
#define _plxwritereg_1(sc, offset, val) \
    bus_space_write_1(sc->plx_t, sc->plx_h, offset, val)

#define _plxreadlocal0_4(sc, offset) \
    bus_space_read_4(sc->reg_t, sc->reg_h, offset)
#define _plxwritelocal0_4(sc, offset, val) \
    bus_space_write_4(sc->reg_t, sc->reg_h, offset, val)

#define _plxreadlocal1_4(sc, offset) \
    bus_space_read_4(sc->rem_t, sc->rem_h, offset)
#define _plxwritelocal1_4(sc, offset, val) \
    bus_space_write_4(sc->rem_t, sc->rem_h, offset, val)
#define _plxreadlocal1_2(sc, offset) \
    bus_space_read_2(sc->rem_t, sc->rem_h, offset)
#define _plxwritelocal1_2(sc, offset, val) \
    bus_space_write_2(sc->rem_t, sc->rem_h, offset, val)

#if _BYTE_ORDER == _LITTLE_ENDIAN
#   undef  __BIG_ENDIAN
#   define __LITTLE_ENDIAN
#elif _BYTE_ORDER == _BIG_ENDIAN
#   undef  __LITTLE_ENDIAN
#   define __BIG_ENDIAN
#else
#   error UNKNOWN ENDIAN (bsd)
#endif

#define DECLARE_SPINLOCKFLAGS(s) int s;
#define SPIN_LOCK_IRQSAVE(lock, s) do { \
            s=splbio(); \
            simple_lock(&lock); \
        } while (0)
#define SPIN_UNLOCK_IRQRESTORE(lock, s) do { \
            simple_unlock(&lock); \
            splx(s); \
        } while (0)

#define SEM_LOCK(sem) lockmgr(&(sem), LK_EXCLUSIVE, 0)
#define SEM_UNLOCK(sem) lockmgr(&(sem), LK_RELEASE, 0)

#define KFREE(p) free(p, M_DEVBUF)
#define KMALLOC(s) malloc(s, M_DEVBUF, M_WAITOK)

#define ACCESS_OK(buf, count, write) \
    uvm_useracc(buf, count, write?B_READ:B_WRITE)

#define COPY_TO_USER(dest, src, size) \
    copyout(src, dest, size)

/*
 * copied from linux/include/linux/list.h
 */
struct list_head {
	struct list_head *next, *prev;
};

#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void
list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define list_for_each(pos, head) \
        for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_entry(ptr, type, member) container_of(ptr, type, member)

typedef int irqreturn_t;
#define IRQ_NONE 0
#define IRQ_HANDLED 1

#define LINUX_RETURN(x) return (x)

#endif

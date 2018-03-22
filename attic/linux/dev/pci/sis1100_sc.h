/* $ZEL: sis1100_sc.h,v 1.3 2002/11/21 13:02:27 wuestner Exp $ */

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

#ifndef _sis1100var_h_
#define _sis1100var_h_

#include <linux/timer.h>
#include <linux/tqueue.h>

#ifdef CONFIG_DEVFS_FS
#   define USE_DEVFS
/*#   undef USE_DEVFS*/
#else
#   undef USE_DEVFS
#endif

#ifdef USE_DEVFS
#include <linux/devfs_fs_kernel.h>
#endif


#include <dev/pci/plx9054reg.h>
#include <dev/pci/sis1100_map.h>
#include <dev/pci/sis3100_map.h>
#include <dev/pci/sis1100_var.h>

#define MAX_SIS1100_DEVICES 8

#define MAXSIZE_KIO 524288

enum sis1100_subdevs {sis1100_subdev_vme, sis1100_subdev_ram,
    sis1100_subdev_sharc};

struct SIS1100_dmabuf {
    size_t size;
    void* cpu_addr;
    dma_addr_t dma_handle;
};

enum irqs_got {got_dma0=1, got_dma1=2, got_end=4, got_eot=8,
        got_xoff=16, got_sync=32, got_l_err=64};

struct irq_vects {
    u_int32_t vector;
    int valid;
};

struct SIS1100_fdata;

struct SIS1100_softc {
    volatile u_int8_t *plxmembase, *plxlocalbase0, *plxlocalbase1;
    u_int32_t plxmemlen, plxlocallen0, plxlocallen1;
    int unit;
    u_int32_t local_ident, remote_ident;
    volatile int remote_ok, old_remote_ok;
    volatile u_int32_t doorbell;
    volatile int got_irqs;
    struct pci_dev *pcidev;
    wait_queue_head_t sis1100_wait;
    wait_queue_head_t irq_wait;
    struct irq_vects irq_vects[8];
    int pending_irqs;
    loff_t sdram_size, sharc_size;
    int sharc_present;
    struct timer_list link_up_timer;
    struct tq_struct link_up_task;
    struct tq_struct vme_irq_task;
    struct semaphore sem_hw;         /* protects hardware */
    struct semaphore sem_fdata_list; /* protects fdata_list_head */
    spinlock_t lock_intcsr;          /* protects INTCSR of PLX */
    struct list_head fdata_list_head;
#ifdef USE_DEVFS
    devfs_handle_t devfs_dev, devfs_dev_sd, devfs_dev_sh;
#endif
    struct kiobuf* iobuf;
    struct SIS1100_dmabuf descbuf;
};

struct SIS1100_fdata {
    struct list_head list;
    struct SIS1100_softc* sc;
    enum sis1100_subdevs subdev;
    int32_t vmespace_am;
    u_int32_t vmespace_datasize;
    int big_endian;            /* 0: little, 1: big */
    int fifo_mode;
    int last_prot_err;
    int owned_irqs;
    int old_remote_ok;
    int sig;
    pid_t pid;
    size_t mindmalen_r, mindmalen_w;
};

#define SIS1100FD(file) ((struct SIS1100_fdata*)(file)->private_data)
#define SIS1100SC(file) (((struct SIS1100_fdata*)(file)->private_data)->sc)

#define ofs(what, elem) ((int)&(((what *)0)->elem))

#define _plxreadreg(sc, offset) readl((sc)->plxmembase+(offset))
#define _plxwritereg(sc, offset, val) writel(val, (sc)->plxmembase+(offset))

#define plxreadreg(sc, reg) \
 _plxreadreg(sc, ofs(struct plx9045reg, reg))
#define plxwritereg(sc, reg, val) \
 _plxwritereg(sc, ofs(struct plx9045reg, reg), val)

#define plxreadlocal0(sc, offset) readl((sc)->plxlocalbase0+(offset))
#define plxwritelocal0(sc, offset, val) writel(val, (sc)->plxlocalbase0+(offset))

#define sis1100readreg(sc, reg) \
 plxreadlocal0(sc, ofs(struct sis1100_reg, reg))
#define sis1100writereg(sc, reg, val) \
 plxwritelocal0(sc, ofs(struct sis1100_reg, reg), val)

#define sis3100readreg(sc, reg, val, locked) \
 sis1100_remote_reg_read(sc, ofs(struct sis3100_reg, reg), val, locked)
#define sis3100writereg(sc, reg, val, locked) \
 sis1100_remote_reg_write(sc, ofs(struct sis3100_reg, reg), val, locked)

#if 0 /* not needed? */
#define plxreadlocal1(sc, offset) (*(volatile u_int32_t*)((sc)->plxlocalbase1+(offset)))
#define plxwritelocal1(sc, offset, val) \
do { \
	*(volatile u_int32_t*)((sc)->plxlocalbase1+(offset)) = (val); \
} while (0)
#endif

/*
#define swap_int(x)  ((((x)>>24)&0x000000ff) |\
                      (((x)>> 8)&0x0000ff00) |\
                      (((x)<< 8)&0x00ff0000) |\
                      (((x)<<24)&0xff000000))

#define swap_short(x) ((((x)>>24)&0x000000ff) |\
                       (((x)>>8)&0x0000ff00))

#define swap_short(x) ((((x)>>8)&0x000000ff) |\
                       (((x)<<8)&0x0000ff00))
*/
extern struct SIS1100_softc *SIS1100_devdata[MAX_SIS1100_DEVICES];

extern struct file_operations SIS1100_fops;

int sis1100_open(struct inode *inode, struct file *file);
int sis1100sdram_open(struct inode *inode, struct file *file);
int sis1100sharc_open(struct inode *inode, struct file *file);

int sis1100_release(struct inode *inode, struct file *file);
int sis1100_ioctl(struct inode *inode, struct file *file,
    	    	unsigned int cmd, unsigned long arg);
loff_t sis1100_llseek(struct file* file, loff_t offset, int orig);
int sis1100_mmap(struct file * file, struct vm_area_struct * vma);
ssize_t sis1100_read(struct file* file, char* buf, size_t count,
    	    	loff_t* ppos);
ssize_t sis1100_write(struct file* file, const char* buf, size_t count,
    	    	loff_t* ppos);

int sis3100sdram_mmap(struct file * file, struct vm_area_struct * vma);

loff_t sis3100sharc_llseek(struct file* file, loff_t offset, int orig);
ssize_t sis3100sharc_read(struct file* file, char* buf, size_t count,
    	    	loff_t* ppos);
ssize_t sis3100sharc_write(struct file* file, const char* buf, size_t count,
    	    	loff_t* ppos);

int SIS1100_intr(struct SIS1100_softc* sc);
int SIS1100_init(struct SIS1100_softc* sc);
void SIS1100_done(struct SIS1100_softc* sc);

void sis1100_synch_handler(unsigned long data);
void sis1100_synch_s_handler(void* data);
void sis1100_vme_irq_handler(void* data);
void sis1100_init_remote(struct SIS1100_softc* sc);
int init_sdram(struct SIS1100_softc* sc);
void dump_glink_status(struct SIS1100_softc* sc, char* text, int locked);
void flush_fifo(struct SIS1100_softc* sc, const char* text, int silent);

int sis1100_disable_irq(struct SIS1100_softc* sc,
    u_int32_t plx_mask, u_int32_t sis_mask);
int sis1100_enable_irq(struct SIS1100_softc* sc,
    u_int32_t plx_mask, u_int32_t sis_mask);
int sis1100_tmp_read(struct SIS1100_softc *sc,
    u_int32_t addr, int32_t am, u_int32_t size, int space, u_int32_t* data);
int sis1100_tmp_write(struct SIS1100_softc *sc,
    u_int32_t addr, int32_t am, u_int32_t size, int space, u_int32_t data);
ssize_t sis1100_read_dma(struct SIS1100_fdata* fd,
    u_int32_t addr, int32_t am, u_int32_t size, int space, int fifo,
    size_t count, u_int8_t* data, int* prot_err);
ssize_t sis1100_read_loop(struct SIS1100_fdata* fd,
    u_int32_t addr, int32_t am, u_int32_t size, int space, int fifo,
    size_t count, u_int8_t* data, int* prot_err);
ssize_t sis1100_write_dma(struct SIS1100_fdata* fd,
    u_int32_t addr, int32_t am, u_int32_t size, int space, int fifo,
    size_t count, const u_int8_t* data, int* prot_err);
ssize_t sis1100_write_loop(struct SIS1100_fdata* fd,
    u_int32_t addr, int32_t am, u_int32_t size, int space, int fifo,
    size_t count, const u_int8_t* data, int* prot_err);
int sis1100_read_pipe(struct SIS1100_softc* sc, struct sis1100_pipe* control);
void sis1100_reset_plx(struct SIS1100_softc* sc);

int sis1100_block_read(struct SIS1100_softc* sc, struct SIS1100_fdata* fd,
    struct sis1100_vme_block_req* req);
int sis1100_block_write(struct SIS1100_softc* sc,struct SIS1100_fdata* fd,
    struct sis1100_vme_block_req* req);

int sis1100_irq_ctl(struct SIS1100_fdata* fd, struct sis1100_irq_ctl* data);
int sis1100_irq_get(struct SIS1100_fdata* fd, struct sis1100_irq_get* data);
int sis1100_irq_wait(struct SIS1100_fdata* fd, struct sis1100_irq_get* data);
int sis1100_irq_ack(struct SIS1100_fdata* fd, struct sis1100_irq_ack* data);
int sis1100_remote_reg_read(struct SIS1100_softc* sc, u_int32_t offs,
    u_int32_t* data, int locked);
int sis1100_remote_reg_write(struct SIS1100_softc* sc, u_int32_t offs,
    u_int32_t data, int locked);
int sis1100_front_io(struct SIS1100_softc* sc, u_int32_t* data, int locked);
int sis1100_front_pulse(struct SIS1100_softc* sc, u_int32_t* data, int locked);
int sis1100_front_latch(struct SIS1100_softc* sc, u_int32_t* data, int locked);
void _front_pulse(struct SIS1100_softc* sc, u_int32_t data);

#endif

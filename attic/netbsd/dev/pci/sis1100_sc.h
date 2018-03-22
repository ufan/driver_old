/* $ZEL: sis1100_sc.h,v 1.8 2003/01/15 14:17:02 wuestner Exp $ */

#ifndef _sis1100sc_h_
#define _sis1100sc_h_

#include "dev/pci/plx9054reg.h"
/*
#include "dev/pci/plx9054_dma.h"
*/
#include "dev/pci/sis1100_map.h"
#include "dev/pci/sis3100_map.h"
#include "dev/pci/sis5100_map.h"
#include "dev/pci/sis1100_var.h"

#define MAX_DMA_LEN 524288 /* 1/2 */

extern struct cfdriver sis1100cfdriver;

enum irqs_got {got_dma0=1, got_dma1=2, got_end=4, got_eot=8,
        got_xoff=16, got_sync=32, got_l_err=64};

struct plx9054dma {
        char *devname;
        bus_space_tag_t iot;
        bus_space_handle_t ioh;
        bus_dma_tag_t dmat;

        int nsegs, rsegs;
        bus_dmamap_t userdma;
        bus_dma_segment_t descsegs;
        bus_dma_segment_t segs;
        struct plx9054_dmadesc *descs; /* kva of descsegs */
        bus_dmamap_t descdma;
        caddr_t kva; /* kva of segs */
};

struct mmapdma {
        bus_dma_tag_t dmat;
        int valid;
        off_t off;
        bus_size_t size;
        bus_dma_segment_t segs;
        bus_dmamap_t dm;
        caddr_t kva; /* kva of segs */
};

struct irq_vects {
    u_int32_t vector;
    int valid;
};

struct SIS1100_fdata {
    enum sis1100_subdev subdev;
    struct proc *p;
    pid_t pid;
    int big_endian;            /* 0: little, 1: big */
    int fifo_mode;
    int32_t vmespace_am;
    u_int32_t vmespace_datasize;
    int last_prot_err;
    int sig;
    int owned_irqs;
    size_t mindmalen_r, mindmalen_w;
    enum sis1100_hw_type old_remote_hw;
    struct mmapdma mmapdma;
};

struct thread_command {
    struct simplelock lock;
    volatile int command;
};

#define PLXDMA_LOCAL2PCI 0
#define PLXDMA_PCI2LOCAL 1
#define PLXDMA_CONSTADDR 2

struct SIS1100_softc {
	struct device sc_dev;
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;
        bus_dma_tag_t sc_dmat;

	bus_space_tag_t plx_t;
	bus_space_handle_t plx_h;
	bus_size_t plx_size;

	bus_space_tag_t reg_t;
	bus_space_handle_t reg_h;
	bus_size_t reg_size;
        bus_addr_t reg_addr;

	bus_space_tag_t rem_t;
	bus_space_handle_t rem_h;
	bus_size_t rem_size;
        bus_addr_t rem_addr;

	void *sc_ih;

        struct plx9054dma sc_dma;

        struct SIS1100_fdata* fdatalist[sis1100_MINORUTMASK+1];

        u_int32_t local_ident, remote_ident;
        volatile int got_irqs;
        volatile u_int32_t doorbell;
        volatile enum sis1100_hw_type remote_hw, old_remote_hw;
        struct irq_vects irq_vects[8];
        int pending_irqs;
        int dsp_present;
        off_t ram_size, dsp_size;
        struct simplelock lock_intcsr;        /* protects INTCSR of PLX */
        struct simplelock lock_sc_inuse;      /* protects sc_inuse */
        struct lock sem_hw;                   /* protects INTCSR of PLX */
        struct callout link_up_timer;
        struct proc* sync_pp;
        struct thread_command sync_command;
        struct proc* vmeirq_pp;
        struct thread_command vmeirq_command;
	int sc_inuse;
        struct simplelock vmeirq_wait;        /* pending_irqs, remote_ok */
        struct simplelock local_wait;
};

#define SIS1100CARD(dev) \
 ((minor(dev)&sis1100_MINORCARDMASK)>>sis1100_MINORCARDSHIFT)
#define SIS1100SC(dev) \
 ((struct SIS1100_softc*)sis1100cfdriver.cd_devs[SIS1100CARD(dev)])
#define SIS1100FD(dev) \
 (struct SIS1100_fdata*)((SIS1100SC(dev)->fdatalist)[minor(dev)&sis1100_MINORUTMASK])

#define ofs(what, elem) ((int)&(((what *)0)->elem))

#define _plxreadreg(sc, offset) \
    bus_space_read_4(sc->plx_t, sc->plx_h, offset)

#define _plxwritereg(sc, offset, val) \
    bus_space_write_4(sc->plx_t, sc->plx_h, offset, val)

#define plxreadreg(sc, reg) \
 _plxreadreg(sc, ofs(struct plx9045reg, reg))

#define plxwritereg(sc, reg, val) \
 _plxwritereg(sc, ofs(struct plx9045reg, reg), val)

#define plxreadlocal0(sc, offset) \
    bus_space_read_4(sc->reg_t, sc->reg_h, offset)

#define plxwritelocal0(sc, offset, val) \
    bus_space_write_4(sc->reg_t, sc->reg_h, offset, val)

#define sis1100readreg(sc, reg) \
 plxreadlocal0(sc, ofs(struct sis1100_reg, reg))
#define sis1100writereg(sc, reg, val) \
 plxwritelocal0(sc, ofs(struct sis1100_reg, reg), val)

#define sis1100readremreg(sc, reg, val, locked) \
 sis1100_remote_reg_read(sc, ofs(struct sis1100_reg, reg), val, locked)
#define sis1100writeremreg(sc, reg, val, locked) \
 sis1100_remote_reg_write(sc, ofs(struct sis1100_reg, reg), val, locked)

#define sis3100readremreg(sc, reg, val, locked) \
 sis1100_remote_reg_read(sc, ofs(struct sis3100_reg, reg), val, locked)
#define sis3100writeremreg(sc, reg, val, locked) \
 sis1100_remote_reg_write(sc, ofs(struct sis3100_reg, reg), val, locked)

#define sis5100readremreg(sc, reg, val, locked) \
 sis1100_remote_reg_read(sc, ofs(struct sis5100_reg, reg), val, locked)
#define sis5100writeremreg(sc, reg, val, locked) \
 sis1100_remote_reg_write(sc, ofs(struct sis5100_reg, reg), val, locked)

int sis1100_init(struct SIS1100_softc *);
void sis1100_done(struct SIS1100_softc *);
int sis1100_intr(void*);

int sis1100_open(dev_t, int, int, struct proc*);
int sis1100_close(dev_t, int, int, struct proc*);
int sis1100_ioctl(dev_t, u_long, caddr_t, int, struct proc*);
int sis1100_read(dev_t dev, struct uio* uio, int f);
int sis1100_write(dev_t dev, struct uio* uio, int f);
paddr_t sis1100_mmap(dev_t dev, off_t off, int prot);
int dma_alloc(struct SIS1100_softc *sc, struct SIS1100_fdata* fd,
    struct sis1100_dma_alloc* d);
int dma_free(struct SIS1100_softc *sc, struct SIS1100_fdata* fd,
    struct sis1100_dma_alloc* d);

int sis1100_enable_irq(struct SIS1100_softc*, u_int32_t, u_int32_t);
int sis1100_disable_irq(struct SIS1100_softc*, u_int32_t, u_int32_t);

int sis1100_remote_reg_read(struct SIS1100_softc* sc, u_int32_t offs,
    u_int32_t* data, int locked);
int sis1100_remote_reg_write(struct SIS1100_softc* sc, u_int32_t offs,
    u_int32_t data, int locked);

void sis1100_init_remote(struct SIS1100_softc* sc);
int  sis1100_remote_init(struct SIS1100_softc* sc);
int  sis3100_remote_init(struct SIS1100_softc* sc);
int  sis5100_remote_init(struct SIS1100_softc* sc);
int  init_sdram(struct SIS1100_softc* sc);

int sis1100_front_io(struct SIS1100_softc* sc, u_int32_t* data, int locked);
int sis1100_front_pulse(struct SIS1100_softc* sc, u_int32_t* data, int locked);
int sis1100_front_latch(struct SIS1100_softc* sc, u_int32_t* data, int locked);

void dump_glink_status(struct SIS1100_softc* sc, char* text, int locked);
void flush_fifo(struct SIS1100_softc*, const char*, int);
void sis1100_reset_plx(struct SIS1100_softc* sc);

int sis1100_read_pipe(struct SIS1100_softc* sc, struct sis1100_pipe* control);
int sis1100_tmp_read(struct SIS1100_softc *sc,
    u_int32_t addr, int32_t am, u_int32_t size, int space, u_int32_t* data);
int sis1100_tmp_write(struct SIS1100_softc *sc,
    u_int32_t addr, int32_t am, u_int32_t size, int space, u_int32_t data);
ssize_t sis1100_read_dma(struct SIS1100_softc*, struct SIS1100_fdata* fd,
    u_int32_t addr, int32_t am, u_int32_t size, int space, int fifo,
    size_t count, u_int8_t* data, int* prot_err);
ssize_t sis1100_write_dma(struct SIS1100_softc*, struct SIS1100_fdata* fd,
    u_int32_t addr, int32_t am, u_int32_t size, int space, int fifo,
    size_t count, const u_int8_t* data, int* prot_err);
int sis1100_writepipe(struct SIS1100_softc* sc, int32_t am,
    int space, int num, u_int32_t* data);
int sis1100_block_read(struct SIS1100_softc* sc, struct SIS1100_fdata* fd,
    struct sis1100_vme_block_req* req);
int sis1100_block_write(struct SIS1100_softc* sc,struct SIS1100_fdata* fd,
    struct sis1100_vme_block_req* req);

int sis1100_irq_ctl(struct SIS1100_softc*, struct SIS1100_fdata* fd,
    struct sis1100_irq_ctl* data);
int sis1100_irq_get(struct SIS1100_softc*, struct SIS1100_fdata* fd,
    struct sis1100_irq_get* data);
int sis1100_irq_wait(struct SIS1100_softc*, struct SIS1100_fdata* fd,
    struct sis1100_irq_get* data);
int sis1100_irq_ack(struct SIS1100_softc*, struct SIS1100_fdata* fd,
    struct sis1100_irq_ack* data);

void sis1100thread_sync(void*);
void sis1100thread_vmeirq(void*);
void sis1100_synch_handler(void*);
void sis1100_vme_irq_handler(void* data);

#endif

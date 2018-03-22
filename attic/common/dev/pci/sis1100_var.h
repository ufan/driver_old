/* $ZEL: sis1100_var.h,v 1.6 2003/01/16 08:42:45 wuestner Exp $ */
#ifndef _sis1100_var_h_
#define _sis1100_var_h_

#define SIS1100_MinVersion 0
#define SIS1100_MajVersion 2
#define SIS1100_Version (SIS1100_MinVersion|(SIS1100_MajVersion<<16))

#ifdef __linux__

#include "linux/ioctl.h"

#else

#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/proc.h>

#endif

/*
 * minorbits:
 * ccttuuuu
 */

/* used bits of minor number */
#define sis1100_MINORBITS 8
/* bits used for cardnumber, number_of_cards<=(1<<MINORCARDBITS) */
#define sis1100_MINORCARDBITS 2
/* type 0: VME; type 1: SDRAM; type 2: sis1100_control; type 3: dsp*/
#define sis1100_MINORTYPEBITS 2

#define sis1100_MAXCARDS (1<<sis1100_MINORCARDBITS)
#define sis1100_MINORUSERBITS (sis1100_MINORBITS-sis1100_MINORCARDBITS-sis1100_MINORTYPEBITS)

#define sis1100_MINORCARDSHIFT (sis1100_MINORUSERBITS+sis1100_MINORTYPEBITS)
#define sis1100_MINORCARDMASK (((1<<sis1100_MINORCARDBITS)-1)<<(sis1100_MINORUSERBITS+sis1100_MINORTYPEBITS))
#define sis1100_MINORTYPESHIFT sis1100_MINORUSERBITS
#define sis1100_MINORTYPEMASK (((1<<sis1100_MINORTYPEBITS)-1)<<sis1100_MINORUSERBITS)
#define sis1100_MINORUSERSHIFT 0
#define sis1100_MINORUSERMASK ((1<<sis1100_MINORUSERBITS)-1)
#define sis1100_MINORUTMASK (sis1100_MINORTYPEMASK|sis1100_MINORUSERMASK)

enum sis1100_subdev {sis1100_subdev_remote, sis1100_subdev_ram,
    sis1100_subdev_ctrl, sis1100_subdev_dsp};
enum sis1100_hw_type {
    sis1100_hw_invalid=0,
    sis1100_hw_pci=1,
    sis1100_hw_vme=2,
    sis1100_hw_camac=3,
    sis1100_hw_lvd=4
};

struct sis1100_vme_req {
    int size;
    int32_t am;
    u_int32_t addr;
    u_int32_t data;
    u_int32_t error;
};

/*
 * struct sis1100_vme_req {
 *     int size;
 *     int32_t am;
 *     u_int32_t addr;
 *     union {
 *         u_int8_t u8;
 *         u_int16_t u16;
 *         u_int32_t u32;
 *         u_int64_t u64;
 *     } data;
 *     u_int32_t error;
 * };
 */

struct sis1100_vme_block_req {
    int size;        /* size of dataword */
    int fifo;
    size_t num;      /* number of datawords */
    int32_t am;
    u_int32_t addr;  /* remote bus address */
    u_int32_t* data; /* local user space address */
    u_int32_t error;
};

struct sis1100_vme_super_block_req {
    int n;
    struct sis1100_vme_block_req* reqs;
};

struct sis1100_camac_req {
    u_int32_t N;
    u_int32_t A;
    u_int32_t F;
    u_int32_t data;
    u_int32_t error;
};

struct sis1100_camac_scan_req {
    u_int32_t N;
    u_int32_t A;
    u_int32_t F;
    u_int32_t data;
    u_int32_t error;
};

struct sis1100_pipelist {
    u_int32_t head; /* masked with 0xff3f0400:                  */
    	    	    /* only 'be', remote space and w/r are used */
    int32_t am;
    u_int32_t addr;
    u_int32_t data; /* only used for write access */
};

struct sis1100_pipe {
    int num;
    struct sis1100_pipelist* list;
    u_int32_t* data;
    u_int32_t error;
};

struct sis1100_writepipe {
    int num;
    int am;
    u_int32_t* data; /* num*{addr, data} */
    u_int32_t error;
};

struct vmespace {
    int32_t am;
    u_int32_t datasize;
    int swap;          /* 1: swap words 0: don't swap -1: not changed */
    int mapit;         /* not used */
    ssize_t mindmalen; /* 0: never use DMA; 1: always use DMA; -1: not changed */
};

struct sis1100_ident_dev {
    enum sis1100_hw_type hw_type;
    int hw_version;
    int fw_type;
    int fw_version;
};

struct sis1100_ident {
    struct sis1100_ident_dev local;
    struct sis1100_ident_dev remote;
    int remote_ok;
    int remote_online;
};

struct sis1100_ctrl_reg {
    int offset;
    u_int32_t val;
    u_int32_t error;
};

struct sis1100_irq_ctl {
    u_int32_t irq_mask;
    int signal;
};

struct sis1100_irq_get {
    u_int32_t irq_mask;
    int remote_status; /* -1: down 1: up 0: unchanged */
    u_int32_t irqs;
    int level;
    int32_t vector;
};

struct sis1100_irq_ack {
    u_int32_t irq_mask;
};

struct sis1100_dma_alloc {
    size_t size;
    off_t offset;
    u_int32_t dma_addr;
};

#define SIS3100_VME_IRQS      0xFE
#define SIS3100_FLAT_IRQS    0xF00
#define SIS3100_LEMO_IRQS   0x7000
#define SIS3100_DSP_IRQ     0x8000
#define SIS3100_FRONT_IRQS (SIS3100_FLAT_IRQS  | SIS3100_LEMO_IRQS)
#define SIS3100_EXT_IRQS   (SIS3100_FRONT_IRQS | SIS3100_DSP_IRQ  )
#define SIS3100_IRQS       (SIS3100_VME_IRQS   | SIS3100_EXT_IRQS )

#define SIS5100_LAM_IRQS         0
#define SIS5100_FLAT_IRQS        0
#define SIS5100_LEMO_IRQS        0
#define SIS5100_DSP_IRQ          0
#define SIS5100_FRONT_IRQS (SIS5100_FLAT_IRQS  | SIS5100_LEMO_IRQS)
#define SIS5100_EXT_IRQS   (SIS5100_FRONT_IRQS | SIS5100_DSP_IRQ  )
#define SIS5100_IRQS       (SIS5100_LAM_IRQS   | SIS5100_EXT_IRQS )  

#define SIS1100_FRONT_IRQS 0x30000
#define SIS1100_IRQS SIS1100_FRONT_IRQS

#define GLINK_MAGIC 'g'

#define SIS1100_SETVMESPACE     _IOW(GLINK_MAGIC,  1, struct vmespace)
#define SIS3100_VME_PROBE       _IOW(GLINK_MAGIC,  2, u_int32_t)
#define SIS3100_VME_READ        _IOWR(GLINK_MAGIC, 3, struct sis1100_vme_req)
#define SIS3100_VME_WRITE       _IOWR(GLINK_MAGIC, 4, struct sis1100_vme_req)
#define SIS3100_VME_BLOCK_READ  _IOWR(GLINK_MAGIC, 5, struct sis1100_vme_block_req)
#define SIS3100_VME_BLOCK_WRITE _IOWR(GLINK_MAGIC, 6, struct sis1100_vme_block_req)
#define SIS1100_LOCAL_CTRL_READ _IOWR(GLINK_MAGIC, 7, struct sis1100_ctrl_reg)
#define SIS1100_LOCAL_CTRL_WRITE _IOWR(GLINK_MAGIC, 8, struct sis1100_ctrl_reg)
#define SIS1100_REMOTE_CTRL_READ _IOWR(GLINK_MAGIC, 9, struct sis1100_ctrl_reg)
#define SIS1100_REMOTE_CTRL_WRITE _IOWR(GLINK_MAGIC, 10, struct sis1100_ctrl_reg)
#define SIS1100_PIPE            _IOWR(GLINK_MAGIC, 11, struct sis1100_pipe)
#define SIS1100_MAPSIZE         _IOR(GLINK_MAGIC,  12, u_int32_t)
#define SIS1100_LAST_ERROR      _IOR(GLINK_MAGIC,  13, u_int32_t)
#define SIS1100_IDENT           _IOR(GLINK_MAGIC,  14, struct sis1100_ident)
#define SIS1100_FIFOMODE        _IOWR(GLINK_MAGIC, 15, int)

#define SIS1100_IRQ_CTL         _IOW(GLINK_MAGIC,  17, struct sis1100_irq_ctl)
#define SIS1100_IRQ_GET         _IOWR(GLINK_MAGIC, 18, struct sis1100_irq_get)
#define SIS1100_IRQ_ACK         _IOW(GLINK_MAGIC,  19, struct sis1100_irq_ack)
#define SIS1100_IRQ_WAIT        _IOWR(GLINK_MAGIC, 20, struct sis1100_irq_get)

#define SIS1100_MINDMALEN       _IOWR(GLINK_MAGIC, 21, int[2])

#define SIS1100_FRONT_IO        _IOWR(GLINK_MAGIC, 22, u_int32_t)
#define SIS1100_FRONT_PULSE     _IOW(GLINK_MAGIC,  23, u_int32_t)
#define SIS1100_FRONT_LATCH     _IOWR(GLINK_MAGIC, 24, u_int32_t)

#define SIS3100_VME_SUPER_BLOCK_READ _IOWR(GLINK_MAGIC, 25, struct sis1100_vme_super_block_req)
#define SIS3100_WRITE_PIPE      _IOWR(GLINK_MAGIC, 26, struct sis1100_writepipe)

#define SIS1100_DMA_ALLOC       _IOWR(GLINK_MAGIC, 27, struct sis1100_dma_alloc)
#define SIS1100_DMA_FREE        _IOW(GLINK_MAGIC,  28, struct sis1100_dma_alloc)

#define SIS5100_CCCZ            _IO  (GLINK_MAGIC, 29)
#define SIS5100_CCCC            _IO  (GLINK_MAGIC, 30)
#define SIS5100_CCCI            _IOW (GLINK_MAGIC, 31, int)
#define SIS5100_CNAF            _IOWR(GLINK_MAGIC, 32, struct sis1100_camac_req)
#define SIS1100_SWAP            _IOWR(GLINK_MAGIC, 33, int)
#define SIS3100_TIMEOUTS        _IOWR(GLINK_MAGIC, 34, int[2])

/* the following functions (1xx) are not designed for "daily use",
   but will be usefull */
#define SIS1100_RESET           _IO(GLINK_MAGIC,  102)
#define SIS1100_REMOTE_RESET    _IO(GLINK_MAGIC,  103)
#define SIS1100_DEVTYPE         _IOR(GLINK_MAGIC, 104, enum sis1100_subdev)
#define SIS1100_DRIVERVERSION   _IOR(GLINK_MAGIC, 105, int)

/* the following function(s) (2xx) exists only for development purposes and will
   be deleted soon */
#define SIS1100_RAM_BLOCK_READ  _IOWR(GLINK_MAGIC, 200, struct sis1100_vme_block_req)

#ifndef PURE_SIS1100_NAMESPACE
#define SETVMESPACE SIS1100_SETVMESPACE
#define VME_PROBE SIS3100_VME_PROBE
#endif

#define SIS1100_CONTROL_WRITE SIS1100_LOCAL_CTRL_WRITE

#endif

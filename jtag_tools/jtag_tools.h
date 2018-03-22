/*
 * $ZEL: jtag_tools.h,v 1.1 2003/08/30 21:23:19 wuestner Exp $
 */

#ifndef _jtag_tools_h_
#define _jtag_tools_h_

#include <sys/types.h>

#define TDI     0x01
#define TMS     0x02
#define TCK     0x04    /* use only if autoclock disabled */
#define TDO     0x08
#define JT_C    0x300   /* autoclock, enable */

#define BYPASS  0xFF
#define IDCODE  0xFE
#define ISPEN   0xE8
#define FPGM    0xEA    /* Programs specific bit values at specified addresses */
#define FADDR   0xEB    /* Sets the PROM array address register */
#define FVFY1   0xF8    /* Reads the fuse values at specified addresses */
#define NORMRST	0xF0    /* Exits ISP Mode ? */
#define FERASE  0xEC    /* Erases a specified program memory block */
#define SERASE  0x0A    /* Globally refines the programmed values in the array */
#define FDATA0  0xED    /* Accesses the array word-line register ? */
#define FDATA3  0xF3    /* 6 */
#define CONFIG  0xEE

#define VID_AMD              1
#define VID_INTEL            9
#define VID_DEC             53
#define VID_XILINX          73
#define VID_COMPAQ          74
#define VID_ANALOG_DEVICES 101
#define VID_ALTERA         110
#define VID_VITESSE        116
/* AMCC 112 (bank 2) */

#define DID_XC18V01 0x5024 /* 1 Mbit */
#define DID_XC18V02 0x5025 /* 2 Mbit */
#define DID_XC18V04 0x5026 /* 4 Mbit */
#define DID_XCV150  0x0618
#define DID_XCV400  0x0628

#define ID_XC18V01 ((DID_XC18V01<<12)|(VID_XILINX<<1)|1)
#define ID_XC18V02 ((DID_XC18V02<<12)|(VID_XILINX<<1)|1)
#define ID_XC18V04 ((DID_XC18V04<<12)|(VID_XILINX<<1)|1)
#define ID_XCV150  ((DID_XCV150 <<12)|(VID_XILINX<<1)|1)
#define ID_XCV400  ((DID_XCV400 <<12)|(VID_XILINX<<1)|1)

struct jtag_devdata {
    u_int32_t vendor_id;
    u_int32_t part_id;
    int ir_len;
    const char* name;
};

struct jtag_tab;
struct jtag_dev {
    struct jtag_tab* chain;
    u_int32_t	id;
    const char* name;
    int version;
    int ir_len;	/* instruction register */
    int pre_c_bits;
    int after_c_bits;
    int pre_d_bits;
    int after_d_bits;
};

struct jtag_tab {
    int p;
    int num_devs;
    int state;  /* -1: unknown device (corrupted JTAG chain) */
                /*  0: state is RTI (RUN-TEST/IDLE) */
                /*  1: state is CAPTURE-DR */
    struct jtag_dev* devs;
};

extern int debug;
extern int simulate;

void clear_jtag_tab(struct jtag_tab* tab);
int jtag_init(struct jtag_tab* jtag_tab);
void jtag_print_ids(struct jtag_tab* jtab);
int jtag_read_XC18V00(struct jtag_dev* dev, void* data, int size);
int jtag_write_XC18V00(struct jtag_dev* dev, void* data, int size);

const char* newstate(int tms, int* count);

#endif

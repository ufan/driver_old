/*
 * $ZEL: jtag_tools.c,v 1.3 2003/09/01 10:48:29 wuestner Exp $
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define __CONCAT(x,y)	x ## y
#define __CC(x, y) __CONCAT(x,y)
#define __CONCAT3(x,y,z) __CC(x, __CC(y,z))
#define __STRING(x)	#x
#define __SS(s) __STRING(s)

#include __SS(__CONCAT3(../dev/pci/, DRIVER, _var.h))
#include "jtag_tools.h"

int debug;
int simulate;

#define __CONCAT(x,y)	x ## y
#define __STRING(x)	#x
#define __SS(s) __STRING(s)
#define JTAG_DEV(vendor, name, ir_len) { \
    __CONCAT(VID_, vendor),              \
    __CONCAT(DID_, name),                \
    ir_len,                              \
    __SS(name)                           \
}
#define JTAG_DEV_END(name) {             \
    0, 0, -1, __SS(name)                 \
}

static const struct jtag_devdata jtag_devdata[]={
    JTAG_DEV(XILINX, XC18V01, 8),
    JTAG_DEV(XILINX, XC18V02, 8),
    JTAG_DEV(XILINX, XC18V04, 8),
    JTAG_DEV(XILINX, XCV150 , 5),
    JTAG_DEV(XILINX, XCV400 , 5),
    JTAG_DEV_END(unknown device)
};

/****************************************************************************/
static int
find_jtag_devicedata(struct jtag_dev* dev)
{
    const struct jtag_devdata* d=jtag_devdata;
    while (d->vendor_id) {
        u_int32_t id=(d->part_id<<12)|(d->vendor_id<<1)|0x1;
        if ((dev->id&0x0fffffff)==id) {
            dev->name=d->name;
            dev->ir_len=d->ir_len;
            dev->version=(dev->id>>28)&0xf;
            return 0;
        }
        d++;
    }
    dev->name=d->name;
    dev->ir_len=d->ir_len;
    return 1;
}

void
clear_jtag_tab(struct jtag_tab* tab)
{
    free(tab->devs);
    tab->num_devs=0;
}

static int
add_jtag_device(struct jtag_tab* tab, u_int32_t id)
{
    int i;
    struct jtag_dev* help=malloc((tab->num_devs+1)*sizeof(struct jtag_dev));
    if (!help) {
        fprintf(stderr, "alloc %d bytes: %s\n",
            (tab->num_devs+1)*sizeof(struct jtag_dev),
            strerror(errno));
        return -1;
    }
    for (i=tab->num_devs; i>0; i--) help[i]=tab->devs[i-1];
    help[0].id=id;
    free(tab->devs);
    tab->devs=help;
    tab->num_devs++;
    tab->devs[0].chain=tab;
    return find_jtag_devicedata(tab->devs);
}
/****************************************************************************/
static int
jt_putcsr(struct jtag_tab* jdev, u_int32_t v, const char* q, int s)
{
    const char* state;
    int p=jdev->p;
    int count;

    if (!s) {
        if (ioctl(p, __CC(IONAME, _JTAG_PUT), &v)<0) {
            fprintf(stderr, "ioctl(JTAG_PUT, 0x%x): %s\n", v, strerror(errno));
            return -1;
        }
    }

    state=newstate(v&TMS, &count);
    if (debug)
        fprintf(stderr, "put %s %s %s: %s %2d\n",
                v&TMS?"TMS":"   ", v&TDI?"TDI":"   ", q,
                state, count);
    return 0;
}

static u_int32_t
jt_getcsr(struct jtag_tab* jdev, int s)
{
    u_int32_t v;
    int p=jdev->p;

    if (!s) {
        if (ioctl(p, __CC(IONAME, _JTAG_GET), &v)<0) {
            fprintf(stderr, "ioctl(JTAG_GET): %s\n", strerror(errno));
            return -1;
        }
    } else {
        v=0;
    }
    if (debug)
        fprintf(stderr, "got %s %s\n", v&TDO?"TDO":"   ", v&TDI?"TDI":"   ");
    return v;
}

static u_int32_t
jt_data(struct jtag_tab* jdev, int s)
{
    u_int32_t v;
    int p=jdev->p;

    if (!s) {
        if (ioctl(p, __CC(IONAME, _JTAG_DATA), &v)<0) {
            fprintf(stderr, "ioctl(JTAG_DATA): %s\n", strerror(errno));
            return -1;
        }
    }
    if (debug)
        fprintf(stderr, "got data: 0x%08x\n", v);
    return v;
}
/****************************************************************************/
static u_int32_t
jtag_data(struct jtag_dev* dev, u_int32_t din, int len, int s)
{
    struct jtag_tab* jdev=dev->chain;
    u_int32_t tdo, xtdo, ms, ms1;
    int i;

    if (debug) fprintf(stderr, "data(0x%x, %d)\n", din, len);

    jt_putcsr(jdev, JT_C|TMS|TDI, "d_", s);
    jt_putcsr(jdev, JT_C|TDI, "d_", s);
    jt_putcsr(jdev, JT_C|TDI, "d_", s);			/* SHIFT-DR */
    
    for (i=dev->pre_d_bits; i>0; i--) {
        jt_putcsr(jdev, JT_C|TDI, "d-", s);
    }
    ms=0; tdo=0; xtdo=1;
    ms1=dev->after_d_bits?0:TMS;
    for (i=len-1; i>=0; i--) {
        tdo|=(jt_getcsr(jdev, s)&TDO)?xtdo:0; xtdo<<=1;
        if (!i) ms=ms1;
        jt_putcsr(jdev, JT_C|ms|(din&TDI), "d+", s);/*does only work if TDI==0x1*/
        din>>=1;
    }
    for (i=dev->after_d_bits-1; i>=0; i--) {
        if (!i) ms=TMS;
        jt_putcsr(jdev, JT_C|ms|TDI, "d-", s);
    }

    jt_putcsr(jdev, JT_C|TMS|TDI, "d_", s);
    jt_putcsr(jdev, JT_C|TDI, "d_", s);	/* state RTI */
    return tdo;
}
/****************************************************************************/
static int
jtag_rd_data(struct jtag_dev* dev, void *buf, int len, int s)
{
    struct jtag_tab* jdev=dev->chain;
    int i, j;

    if (len == 0) {
        if (jdev->state == 1) {
            jt_putcsr(jdev, JT_C|TMS|TDI, "r_", s);
            jt_putcsr(jdev, JT_C|TMS|TDI, "r_", s);
            jt_putcsr(jdev, JT_C|TDI, "r_", s);
            jdev->state=0;                          /* state RTI */
        }
        return 0;
    }

    if (jdev->state == 0) {
        jt_putcsr(jdev, JT_C|TMS|TDI, "r_", s);
        jt_putcsr(jdev, JT_C|TDI, "r_", s);         /* CAPTURE-DR */

        for (i=dev->pre_d_bits; i>0; i--) {
            jt_putcsr(jdev, JT_C|TDI, "d-", s);
        }
        jdev->state=1;
    }

    for (i=(len+3)/4; i; i--) {
        for (j=32; j; j--) jt_putcsr(jdev, JT_C|TDI, "r+", s);
        *((u_int32_t*)buf)++ =jt_data(jdev, s);
    }
    return 0;
}
/****************************************************************************/
static int
jtag_wr_data(struct jtag_dev* dev, void *buf,
    u_int len,      /* Anzahl Byte */
    int	state,	    /* -1: end     */
    int s)
{
    struct jtag_tab* jdev=dev->chain;
    u_int32_t *bf, din;
    u_int8_t ms, ms1;
    int i;

    bf=(u_int32_t*)buf;
    len >>=2;
    if (len ==0) return 0;

    jt_putcsr(jdev, JT_C|TMS|TDI, "w_", s);
    jt_putcsr(jdev, JT_C|TDI, "w_", s);
    jt_putcsr(jdev, JT_C|TDI, "w_", s);


    for (i=dev->pre_d_bits; i>0; i--) {
        jt_putcsr(jdev, JT_C|TDI, "w-", s);
    }

    ms=0;
    ms1=dev->after_d_bits?0:TMS;
    while (len) {
        din=*bf++;
        for (i=31; i>=0; i--) {
            if ((len==0) && !i) ms=ms1;
            jt_putcsr(jdev, JT_C|ms|(din&TDI), "w+", s);
            din>>=1;
        }
        len--;
    }

    for (i=dev->after_d_bits-1; i>=0; i--) {
        if (!i) ms=TMS;
        jt_putcsr(jdev, JT_C|TDI|ms, "w-", s);
    }

    if (state < 0) {
        jt_putcsr(jdev, JT_C|TMS|TDI, "w.", s);
        jt_putcsr(jdev, JT_C|TDI, "w.", s);
        jdev->state=0;
    }

    return 0;
}
/****************************************************************************/
int
jtag_init(struct jtag_tab* jdev)
{
    int i, bits;
    u_int32_t id;

    if (debug) fprintf(stderr, "init\n");
    clear_jtag_tab(jdev);
    jdev->state=0;              /* RTI state when function returns */

    for (i=0; i<5; i++) {           /* TLR state, ID DR wird selectiert */
        jt_putcsr(jdev, JT_C|TMS, "ii", 0);
    }
    jt_putcsr(jdev, JT_C, "ii", 0);             /* RTI state */

/* RTI (Run-Test/Idle) */

    jt_putcsr(jdev, JT_C|TMS, "ii", 0);             /* Select DR */
    jt_putcsr(jdev, JT_C, "ii", 0);                 /* Capture DR */

    do {
        for (i=0; i<32; i++) {  /*  shift DR */
            jt_putcsr(jdev, JT_C, "i+", 0);
        }
        id=jt_data(jdev, 0);
        if (id) {
            int res;
            res=add_jtag_device(jdev, id);
            if (res<0) break;
            if (res>0) jdev->state=-1; /* ir_len for device not known */
        }
    } while (id);

    jt_putcsr(jdev, JT_C|TMS, "ii", 0);             /* Exit1 DR */
    jt_putcsr(jdev, JT_C|TMS, "ii", 0);             /* Update DR */
    jt_putcsr(jdev, JT_C, "ii", 0);                 /* RTI state */

    if (jdev->state) goto raus;

    for (i=0, bits=0; i<jdev->num_devs; i++) {
        struct jtag_dev* dev=jdev->devs+i;
        dev->pre_d_bits=jdev->num_devs-i-1;
        dev->after_d_bits=i;
        dev->after_c_bits=bits;
        bits+=dev->ir_len;
    }
    for (i=jdev->num_devs-1, bits=0; i>=0; i--) {
        struct jtag_dev* dev=jdev->devs+i;
        dev->pre_c_bits=bits;
        bits+=dev->ir_len;
    }

raus:
    return jdev->num_devs;
}
/****************************************************************************/
void
jtag_print_ids(struct jtag_tab* jtab)
{
    int i;

    fprintf(stderr, "JTAG chain\n");
    fprintf(stderr, "-----------\n"); jtab->state=0;
    for (i=0; i<jtab->num_devs; i++) {
        struct jtag_dev* dev=jtab->devs+i;
        fprintf(stderr, "%2d: 0x%08x %-8s  Version %d\n",
                i,
                dev->id, dev->name, dev->version);
    }
    fprintf(stderr, "\n");
}
/****************************************************************************/
static int
jtag_instruction(struct jtag_dev* dev, u_int32_t icode, int s)
{
    struct jtag_tab* jdev=dev->chain;
    u_int32_t tdo, xtdo, ms, ms1;
    int i;

    if (debug) fprintf(stderr, "instruction(0x%x)\n", icode);

    jt_putcsr(jdev, JT_C|TMS|TDI, "c_", s);
    jt_putcsr(jdev, JT_C|TMS|TDI, "c_", s);
    jt_putcsr(jdev, JT_C|TDI, "c_", s);
    jt_putcsr(jdev, JT_C|TDI, "c_", s);

    for (i=dev->pre_c_bits; i>0; i--) {
        jt_putcsr(jdev, JT_C|TDI, "c-", s);
    }
    ms=0; tdo=0; xtdo=1;
    ms1=dev->after_c_bits?0:TMS;
    for (i=dev->ir_len-1; i>=0; i--) {
        tdo|=(jt_getcsr(jdev, s)&TDO)?xtdo:0; xtdo<<=1;
        if (!i) ms=ms1;
        jt_putcsr(jdev, JT_C|ms|(icode&TDI), "c+", s);/*does only work if TDI==0x1*/
        icode>>=1;
    }
    for (i=dev->after_c_bits-1; i>=0; i--) {
        if (!i) ms=TMS;
        jt_putcsr(jdev, JT_C|ms|TDI, "c-", s);
    }
    if (icode == CONFIG) { sleep(1); printf("2\n"); }

    jt_putcsr(jdev, JT_C|TMS|TDI, "c_", s);
    if (icode == CONFIG) { sleep(1); printf("1\n"); }

    jt_putcsr(jdev, JT_C|TDI, "c_", s);
    if (icode == CONFIG) { sleep(1); printf("0\n"); }

    return tdo;
}
/****************************************************************************/
int
jtag_read_XC18V00(struct jtag_dev* dev, void* data, int size)
{
    struct jtag_tab* jdev=dev->chain;
    u_int32_t dout, size0;
    int ret, i;
    int s=0;

    if (jdev->state) {
        fprintf(stderr, "jtag_tab.state=%d\n", jdev->state);
        return -1;
    }

    fprintf(stderr, "reading device %s\n", dev->name);
    switch (dev->id&0x0fffffff) {
        case ID_XC18V01: size0=0x20000; break;
        case ID_XC18V02: size0=0x40000; break;
        case ID_XC18V04: size0=0x80000; break;
        default: fprintf(stderr, "wrong device: %s\n", dev->name); return -1;
    }
    if ((ret=jtag_instruction(dev, IDCODE, s)) < 0) {
        fprintf(stderr, "JTAG not initialized\n");
        return -1;
    }

    dout=jtag_data(dev, -1, 32, s);

    if (((dout&0x0fffffff)!=dev->id)&&!s) {
        fprintf(stderr, "wrong IDCODE: %08X\n", dout);
        return -1;
    }

    if (size!=size0) {
        fprintf(stderr, "wrong buffersize\n");
        return -1;
    }

    jtag_instruction(dev, ISPEN, s);
    jtag_data(dev, 0x34, 6, s);
    ret=jtag_instruction(dev, FADDR, s);
    if ((ret!=0x11)&&!s) fprintf(stderr, "FADDR.3 %02X\n", ret);

    jtag_data(dev, 0, 16, s);
    jtag_instruction(dev, FVFY1, s);
debug=0;

    if (!s) {
        for (i=0; i<50; i++) {      /* mindestens 20 */
            dout +=jt_data(jdev, s);
        }
    }

    if ((ret=jtag_rd_data(dev, data, size, s)) != 0) {
        fprintf(stderr, "\n");
        return ret;
    }
    jtag_rd_data(dev, 0, 0, s);

    return 0;
}
/****************************************************************************/
int
jtag_write_XC18V00(struct jtag_dev* dev, void* data, int size)
{
    struct jtag_tab* jdev=dev->chain;
    u_int32_t dout;
    int	data0, size0x, ret, i, j;
    int s=0;

    if (jdev->state) {
        fprintf(stderr, "jtag_tab.state=%d\n", jdev->state);
        return -1;
    }

    fprintf(stderr, "writing device %s\n", dev->name);

    switch (dev->id) {
        case ID_XC18V01: data0=0x100; size0x=0x20000; break;
        case ID_XC18V02: data0=0x200; size0x=0x40000; break;
        case ID_XC18V04: data0=0x200; size0x=0x80000; break;
        default: fprintf(stderr, "wrong device: %s\n", dev->name); return -1;
    }

    if (size>size0x) {
        fprintf(stderr, "%d bytes is too lage for device %s\n",
                size, dev->name);
        return -1;
    }
    if (size<size0x) {
        fprintf(stderr, "device %s has %d bytes but file has only %d.\n",
                dev->name, size0x, size);
    }

    jtag_instruction(dev, ISPEN, s);
    jtag_data(dev, 0x04, 6, s);

    ret=jtag_instruction(dev, FADDR, s);
    if ((ret!=0x11)&&!s) printf("FADDR.0 %02X\n", ret);
    jtag_data(dev, 1, 16, s);
    jtag_instruction(dev, FERASE, s);

    if (!s) {
        for (i=0; i<1000; i++) {   /* mindestens 500 */
            dout +=jt_data(jdev, s);
        }
    }

    jtag_instruction(dev, NORMRST, s);

    ret=jtag_instruction(dev, BYPASS, s);
    if ((ret!=0x01)&&!s) printf("BYPASS.0 %02X\n", ret);

    jtag_instruction(dev, ISPEN, s);
    jtag_data(dev, 0x04, 6, s);


    for (i=0; i<size/data0; i++, ((u_int8_t*)data)+=data0) {
        jtag_instruction(dev, FDATA0, s);
        if ((ret=jtag_wr_data(dev, data, data0, -1, s)) != 0) return ret;

        if (!i) {
            ret=jtag_instruction(dev, FADDR, s);
            if (ret != 0x11) printf("FADDR.1 %02X\n", ret);
            jtag_data(dev, 0, 16, s);
        }

        jtag_instruction(dev, FPGM, s);

        if (!s) {
            for (j=0; j<1000; j++) {    /* mindestens 500 */
                dout +=jt_data(jdev, s);
            }
        }
        fprintf(stderr, ".");
    }
    fprintf(stderr, "\n");

    ret=jtag_instruction(dev, FADDR, s);
    if ((ret!=0x11)&&!s) printf("FADDR.2 %02X\n", ret);

    jtag_data(dev, 1, 16, s);
    jtag_instruction(dev, SERASE, s);

    if (!s) {
        for (i=0; i<1000; i++) {        /* mindestens 500 */
            dout +=jt_data(jdev, s);
        }
    }

    jtag_instruction(dev, NORMRST, s);

    ret=jtag_instruction(dev, BYPASS, s);
    if ((ret!=0x01)&&!s) printf("BYPASS.1 %02X\n", ret);

    printf("%d byte written\n", size);

    return 0;
}
/****************************************************************************/
/****************************************************************************/

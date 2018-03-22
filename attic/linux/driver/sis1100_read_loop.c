/* $ZEL: sis1100_read_loop.c,v 1.4 2003/01/09 12:13:19 wuestner Exp $ */

#include "Copyright"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/wrapper.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <linux/iobuf.h>
#include <linux/highmem.h>
#include <asm/uaccess.h>

#include <dev/pci/sis1100_sc.h>

/*
 * sis1100_read_loop always transports data directly to userspace!
 * Access permissions have to be checked before.
 */

static ssize_t
_sis1100_read_loop(
    struct SIS1100_fdata* fd,
    u_int32_t addr,           /* VME or SDRAM address */
    int32_t am,               /* address modifier, not used if <0 */
    u_int32_t size,           /* datasize */
    int space,                /* remote space (1,2: VME; 6: SDRAM) */
    int fifo_mode,
    int count,                /* bytes to be transferred */
    u_int8_t* data,           /* source (user virtual address) */
    int* prot_error
    )
{
    struct SIS1100_softc* sc=fd->sc;
    u_int32_t head;
    int idx;

    head=0x00000002|(space&0x3f)<<16;
    down(&sc->sem_hw);
    if (am>=0) {
        head|=0x800;
        sis1100writereg(sc, t_am, am);
    }
    switch (size) {
    case 1:
        for (idx=0; idx<count; idx++, data++) {
            u_int32_t val;
            sis1100writereg(sc, t_hdr, head|(0x01000000<<(addr&3)));
            sis1100writereg(sc, t_adl, addr);
            do {
                *prot_error=sis1100readreg(sc, prot_error);
            } while (*prot_error==0x005);
            if (*prot_error) {
                if (idx>0)
                    count=idx-1;
                else
                    count=-EIO;
                break;
            }
            val=sis1100readreg(sc, tc_dal);
            __put_user((val>>((addr&3)<<3))&0xff, (u_int8_t*)(data));
            if (!fifo_mode) addr++;
        }
        break;
    case 2:
        for (idx=0; idx<count; idx+=2, data+=2) {
            u_int32_t val;
            sis1100writereg(sc, t_hdr, head|(0x03000000<<(addr&3)));
            sis1100writereg(sc, t_adl, addr);
            do {
                *prot_error=sis1100readreg(sc, prot_error);
            } while (*prot_error==0x005);
            if (*prot_error) {
                if (idx>0)
                    count=idx-1;
                else
                    count=-EIO;
                break;
            }
            val=sis1100readreg(sc, tc_dal);
            __put_user((val>>((addr&2)<<3))&0xffff, (u_int16_t*)data);
            if (!fifo_mode) addr+=2;
        }
        break;
    case 4:
        sis1100writereg(sc, t_hdr, head|0x0f000000);
        for (idx=0; idx<count; idx+=4, data+=4) {
            u_int32_t val;
            sis1100writereg(sc, t_adl, addr);
            do {
                *prot_error=sis1100readreg(sc, prot_error);
            } while (*prot_error==0x005);
            if (*prot_error) {
                if (idx>0)
                    count=idx-1;
                else
                    count=-EIO;
                break;
            }
            val=sis1100readreg(sc, tc_dal);
            __put_user(val, (u_int32_t*)data);
            if (!fifo_mode) addr+=4;
        }
        break;
    }
    up(&sc->sem_hw);
    return count;
}

ssize_t
sis1100_read_loop(
    struct SIS1100_fdata* fd,
    u_int32_t addr,           /* VME or SDRAM address */
    int32_t am,               /* address modifier, not used if <0 */
    u_int32_t size,           /* datasize must be 4 for DMA but is not checked*/
    int space,                /* remote space (1,2: VME; 6: SDRAM) */
    int fifo_mode,
    size_t count,             /* bytes to be transferred */
    u_int8_t* data,           /* source (user virtual address) */
    int* prot_err
    )
{
    ssize_t res=1, completed=0;

    *prot_err=0;

    if (!count) return 0;

    if (!access_ok(VERIFY_WRITE, data, count)) return -EFAULT;
    while (count && (res>0) && (*prot_err==0)) {
        res=_sis1100_read_loop(fd, addr, am, size, space, fifo_mode,
                count, data, prot_err);
        if (res>0) {
            if (!fifo_mode) addr+=res;
            data+=res;
            completed+=res;
            count-=res;
        }
    }
    if (completed)
        return completed;
    else
        return res;
}

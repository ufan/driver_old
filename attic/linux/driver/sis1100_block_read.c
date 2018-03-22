/* $ZEL: sis1100_block_read.c,v 1.2 2003/01/09 12:16:04 wuestner Exp $ */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/wrapper.h>
#include <linux/pci.h>
#include <asm/uaccess.h>

#include <dev/pci/sis1100_sc.h>

int
sis1100_block_read (
    struct SIS1100_softc* sc,
    struct SIS1100_fdata* fd,
    struct sis1100_vme_block_req* req
    )
{
    size_t num=req->num;
    int res=0;

    if (!num) {req->error=0; return 0;}
    if (sc->remote_hw==sis1100_hw_invalid) return -ENXIO;
    if (!access_ok(VERIFY_WRITE, req->data, num*req->size))
        return -EFAULT;
    if (num==1) {
        u_int32_t var;
        req->error=sis1100_tmp_read(sc, req->addr, req->am, req->size,
                1/*space*/, &var);
        switch (req->size) {
	case 4: __put_user(var, (u_int32_t*)req->data); break;
	case 2: __put_user(var, (u_int16_t*)req->data); break;
	case 1: __put_user(var, (u_int8_t*)req->data); break;
        }
    } else {
#if LINUX_VERSION_CODE < 0x20500
        if ((req->size==4)&&fd->mindmalen_r&&(num*req->size>=fd->mindmalen_r)) {
            res=sis1100_read_dma(fd, req->addr, req->am, req->size,
                    1/*space*/, req->fifo, num*req->size, (char*)req->data,
                    &req->error);
            req->num=res>>2; /* nonsense if res<0; but does not matter */
        } else
#endif
               {
            res=sis1100_read_loop(fd, req->addr, req->am, req->size,
                    1/*space*/, req->fifo, num*req->size, (char*)req->data,
                    &req->error);
            req->num=res; /* nonsense if res<0; but does not matter */
        }
    }
    return res;
}

int
sis1100_block_write(
    struct SIS1100_softc* sc,
    struct SIS1100_fdata* fd,
    struct sis1100_vme_block_req* req
    )
{
    size_t num=req->num;
    int res=0;

    if (!num) {req->error=0; return 0;}
    if (sc->remote_hw==sis1100_hw_invalid) return -ENXIO;
    if (!access_ok(VERIFY_READ, req->data, num*req->size))
        return -EFAULT;
    if (num==1) {
        u_int32_t data;
        switch (req->size) {
        case 1: __get_user(data, (u_int8_t*)req->data); break;
        case 2: __get_user(data, (u_int16_t*)req->data); break;
        case 4: __get_user(data, (u_int32_t*)req->data); break;
        default: data=0;
        }
        req->error=sis1100_tmp_write(sc, req->addr, req->am, req->size,
                1/*space*/, data);
    } else {
#if LINUX_VERSION_CODE < 0x20500
        if ((req->size==4)&&fd->mindmalen_w&&(num*req->size>=fd->mindmalen_w)) {
            res=sis1100_write_dma(fd, req->addr, req->am, req->size,
                    1/*space*/, req->fifo, num*req->size, (char*)req->data,
                    &req->error);
            req->num=res>>2; /* nonsense if res<0; but does not matter */
        } else
#endif
               {
            res=sis1100_write_loop(fd, req->addr, req->am, req->size,
                    1/*space*/, req->fifo, num*req->size, (char*)req->data,
                    &req->error);
            req->num=res; /* nonsense if res<0; but does not matter */
        }
    }
    return res;
}

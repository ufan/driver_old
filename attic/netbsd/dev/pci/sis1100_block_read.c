/* $ZEL: sis1100_block_read.c,v 1.1 2003/01/15 15:20:06 wuestner Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/sis1100_var.h>
#include <dev/pci/sis1100_sc.h>

static int
check_access(struct SIS1100_softc* sc,
    off_t ppos, off_t maxsize, size_t count, int datasize)
{
    if (ppos>=maxsize) {
    	printf("%s r/w: start addr out of range\n", sc->sc_dev.dv_xname);
    	return EINVAL;
    }

    if ((ppos+count>maxsize)||(ppos+count<ppos)) {
    	printf("%s r/w: end addr out of range\n", sc->sc_dev.dv_xname);
    	return EINVAL;
    }
    
    if ((ppos|count) & (datasize-1)) {
    	printf("%s r/w: unaligned access\n", sc->sc_dev.dv_xname);
    	return EINVAL;
    }
    return 0;
}

int
sis1100_block_read (
    struct SIS1100_softc* sc,
    struct SIS1100_fdata* fd,
    struct sis1100_vme_block_req* req
    )
{
    u_int32_t addr=req->addr;
    int fifo=req->fifo;
    size_t num=req->num;
    int32_t am=-1;
    int space;
    int res=0;
    off_t maxsize;

    if (!sc->remote_hw==sis1100_hw_invalid) return ENXIO;
    if (!num) return 0;

    switch (fd->subdev) {
    case sis1100_subdev_ram:
        if (req->size!=4) return EINVAL;
        if (req->am>0) return EINVAL;
        space=6;
        maxsize=sc->ram_size;
        break;
    case sis1100_subdev_remote: /* VME or CAMAC */
        am=req->am;
        space=1;
        maxsize=0xffffffffU;
        break;
    default:
        return ENOTTY;
    }

    if (check_access(sc, addr, maxsize, num, req->size))
        return EINVAL;

    switch (req->size) {
    case 4: {
        u_int32_t* data=req->data;
        if (!fd->mindmalen_r || (num<fd->mindmalen_r)) {
            u_int32_t tmp;
            while (num) {
                if ((res=sis1100_tmp_read(sc, addr, am, 4, space, &tmp)))
                        break;
                if (suword(data, tmp)) {
                    printf("%s suword failed\n", sc->sc_dev.dv_xname);
                    return EPERM;
                }
                if (!fifo) addr+=4;
                data++;
                num--;
            }
            req->error=res;
            req->num-=num;
        } else {
            res=sis1100_read_dma(sc, fd, addr, am, 4,
                    space, fifo, num*4, (u_int8_t*)data, &req->error);
            if (res>=0)
                req->num=res/4;
            else
                return -res;
        }
    } break;
    case 2: {
        u_int16_t* data=(u_int16_t*)req->data;
        u_int32_t tmp;
        while (num) {
            if ((res=sis1100_tmp_read(sc, addr, am, 2, space, &tmp)))
                    break;
            if (susword(data, tmp)) {
                printf("%s susword failed\n", sc->sc_dev.dv_xname);
                return EPERM;
            }
            if (!fifo) addr+=2;
            data++;
            num--;
        }
        req->error=res;
        req->num-=num;
    } break;
    case 1: {
        u_int8_t* data=(u_int8_t*)req->data;
        u_int32_t tmp;
        while (num) {
            if ((res=sis1100_tmp_read(sc, addr, am, 1, space, &tmp)))
                    break;
            if (subyte(data, tmp)) {
                printf("%s subyte failed\n", sc->sc_dev.dv_xname);
                return EPERM;
            }
            if (!fifo) addr++;
            data++;
            num--;
        }
        req->error=res;
        req->num-=num;
    } break;
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
    u_int32_t addr=req->addr;
    int fifo=req->fifo;
    size_t num=req->num;
    int32_t am=-1;
    int space;
    int res=0;
    off_t maxsize;

    if (sc->remote_hw==sis1100_hw_invalid) return ENXIO;
    if (!num) return 0;

    switch (fd->subdev) {
    case sis1100_subdev_ram:
        if (req->size!=4) return EINVAL;
        if (am>0) return EINVAL;
        space=6;
        maxsize=sc->ram_size;
        break;
    case sis1100_subdev_remote: /* VME or CAMAC */
        am=req->am;
        space=1;
        maxsize=0xffffffffU;
        break;
    default:
        return ENOTTY;
    }

    if (check_access(sc, addr, maxsize, num, req->size))
        return EINVAL;

    switch (req->size) {
    case 4: {
        u_int32_t* data=req->data;
        if (!fd->mindmalen_w || (num<fd->mindmalen_w)) {
            u_int32_t tmp;
            while (num) {
                tmp=fuword(data);
                if ((res=sis1100_tmp_write(sc, addr, am, 4, space, tmp)))
                        break;
                if (!fifo) addr+=4;
                data++;
                num--;
            }
            req->error=res;
            req->num-=num;
        } else {
            res=sis1100_write_dma(sc, fd, addr, am, 4,
                    space, fifo, num*4, (u_int8_t*)data, &req->error);
            if (res>=0)
                req->num=res/4;
            else
                return -res;
        }
    } break;
    case 2: {
        u_int16_t* data=(u_int16_t*)req->data;
        int tmp;
        while (num) {
            if ((tmp=fusword(data))==-1) {
                printf("%s fusword failed\n", sc->sc_dev.dv_xname);
                return EPERM;
            }
            if ((res=sis1100_tmp_write(sc, addr, am, 2, space, tmp)))
                    break;
            if (!fifo) addr+=2;
            data++;
            num--;
        }
        req->error=res;
        req->num-=num;
    } break;
    case 1: {
        u_int8_t* data=(u_int8_t*)req->data;
        int tmp;
        while (num) {
            if ((tmp=fubyte(data))==-1) {
                printf("%s fubyte failed\n", sc->sc_dev.dv_xname);
                return EPERM;
            }
            if ((res=sis1100_tmp_write(sc, addr, am, 1, space, tmp)))
                    break;
            if (!fifo) addr++;
            data++;
            num--;
        }
        req->error=res;
        req->num-=num;
    } break;
    }
    return 0;
}

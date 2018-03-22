/* $ZEL: sis1100_read.c,v 1.8 2003/01/15 14:17:01 wuestner Exp $ */

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
sis1100_read(dev_t dev, struct uio* uio, int f)
{
    struct SIS1100_softc* sc=SIS1100SC(dev);
    struct SIS1100_fdata* fd=SIS1100FD(dev);
    int res;
    int32_t am;
    u_int32_t datasize;
    int space, fifo;
    off_t maxsize;

    if (sc->remote_hw==sis1100_hw_invalid) return ENXIO;
    if (!uio->uio_resid) return 0;

    if (fd->subdev==sis1100_subdev_ram) {
        am=-1;
        datasize=4;
        space=6;
        fifo=0;
        maxsize=sc->ram_size;
    } else {
        am=fd->vmespace_am;
        datasize=fd->vmespace_datasize;
        space=1;
        fifo=fd->fifo_mode;
        maxsize=0xffffffffU;
    }
    if (check_access(sc, uio->uio_offset, maxsize, uio->uio_resid, datasize))
        return EINVAL;

    switch (datasize) {
    case 4: {
        if (!fd->mindmalen_r || (uio->uio_resid<fd->mindmalen_r)) {
            u_int32_t tmp;
            while (uio->uio_resid) {
                res=sis1100_tmp_read(sc, uio->uio_offset, am, 4, space, &tmp);
                if (res) {
                    printf("%s read: error=0x%x\n", sc->sc_dev.dv_xname, res);
                    return EIO;
                }
                res=uiomove((caddr_t)&tmp, 4, uio);
                if (res) {
                    printf("%s uiomove: res=%d\n", sc->sc_dev.dv_xname, res);
                    return res;
                }
            }
        } else {
            /*printf("read/dma: resid=%d\n", uio->uio_resid);*/
            while (uio->uio_resid) {
		struct iovec *iov = uio->uio_iov;
		u_int cnt = iov->iov_len;
                res=sis1100_read_dma(sc, fd, uio->uio_offset, am, datasize,
                    space, 0, cnt, iov->iov_base, &fd->last_prot_err);
                /*
                if (res!=cnt)
                printf("read_dma(offs=%lld, count=%d): res=%d\n",
                        uio->uio_offset, cnt, res);
                */
                if (res==cnt) {
                    uio->uio_offset+=cnt;
                    uio->uio_resid-=cnt;
                    uio->uio_iov++;
                    uio->uio_iovcnt--;
                } else if (res>=0) {
                    uio->uio_offset+=res;
                    uio->uio_resid-=res;
                    iov->iov_base = (caddr_t)iov->iov_base + res;
                    iov->iov_len -= res;
                    return 0;
                } else { /* res<0 */
                    return -res;
                }
            }
        }
    } break;
    case 2: {
        u_int32_t tmp;
        while (uio->uio_resid) {
            res=sis1100_tmp_read(sc, uio->uio_offset, am, 2, space, &tmp);
            if (res) {
                printf("%s read: error=%d\n", sc->sc_dev.dv_xname, res);
                return EIO;
            }
            res=uiomove((caddr_t)&tmp, 2, uio);
            if (res) {
                printf("%s uiomove: res=%d\n", sc->sc_dev.dv_xname, res);
                return res;
            }
        }
    } break;
    case 1: {
        u_int32_t tmp;
        while (uio->uio_resid) {
            res=sis1100_tmp_read(sc, uio->uio_offset, am, 1, space, &tmp);
            if (res) {
                printf("%s read: error=%d\n", sc->sc_dev.dv_xname, res);
                return EIO;
            }
            res=uiomove((caddr_t)&tmp, 1, uio);
            if (res) {
                printf("%s uiomove: res=%d\n", sc->sc_dev.dv_xname, res);
                return res;
            }
        }
    } break;
    }

    return 0;
}

int
sis1100_write(dev_t dev, struct uio* uio, int f)
{
    struct SIS1100_softc* sc=SIS1100SC(dev);
    struct SIS1100_fdata* fd=SIS1100FD(dev);
    int res;
    int32_t am;
    u_int32_t datasize;
    int space, fifo;
    off_t maxsize;
    u_int32_t tmp;
    off_t offset;

    if (sc->remote_hw==sis1100_hw_invalid) return ENXIO;
    if (!uio->uio_resid) return 0;

    if (fd->subdev==sis1100_subdev_ram) {
        am=-1;
        datasize=4;
        space=6;
        fifo=0;
        maxsize=sc->ram_size;
    } else {
        am=fd->vmespace_am;
        datasize=fd->vmespace_datasize;
        space=1;
        fifo=fd->fifo_mode;
        maxsize=0xffffffffU;
    }

    if (check_access(sc, uio->uio_offset, maxsize, uio->uio_resid, datasize))
        return EINVAL;

    if ((datasize==4) && fd->mindmalen_w && (uio->uio_resid>=fd->mindmalen_w)) {
        while (uio->uio_resid) {
	    struct iovec *iov = uio->uio_iov;
	    u_int cnt = iov->iov_len;
            res=sis1100_write_dma(sc, fd, uio->uio_offset, am, datasize,
                space, fifo, cnt, iov->iov_base, &fd->last_prot_err);
            if (res==cnt) {
                uio->uio_offset+=cnt;
                uio->uio_resid-=cnt;
                uio->uio_iov++;
                uio->uio_iovcnt--;
            } else if (res>=0) {
                uio->uio_offset+=res;
                uio->uio_resid-=res;
                iov->iov_base = (caddr_t)iov->iov_base + res;
                iov->iov_len -= res;
                return 0;
            } else { /* res<0 */
                return -res;
            }
        }
        return 0;
    } else {
        while (uio->uio_resid) {
            offset=uio->uio_offset;
            res=uiomove((caddr_t)&tmp, datasize, uio);
            if (res) {
                printf("%s uiomove: res=%d\n", sc->sc_dev.dv_xname, res);
                return res;
            }
            res=sis1100_tmp_write(sc, offset, am, datasize, space, tmp);
            if (res) {
                printf("%s write: error=0x%x\n", sc->sc_dev.dv_xname, res);
                return EIO;
            }
        }
    }
    return 0;
}

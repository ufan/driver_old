/* $ZEL: sis1100_ioctl.c,v 1.6 2003/01/15 14:17:00 wuestner Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <vm/vm.h>
#include <uvm/uvm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

/*#include <dev/pci/sis1100_var.h>*/
#include <dev/pci/sis1100_sc.h>

int
sis1100_ioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
    struct SIS1100_softc* sc=SIS1100SC(dev);
    struct SIS1100_fdata* fd=SIS1100FD(dev);

    switch (cmd) {
	case SIS1100_LOCAL_CTRL_READ: {
#           define d ((struct sis1100_ctrl_reg*)data)
            lockmgr(&sc->sem_hw, LK_EXCLUSIVE, 0);
	    d->val = plxreadlocal0(sc, d->offset&0x7ff);
            lockmgr(&sc->sem_hw, LK_RELEASE, 0);
            d->error=0;
#           undef d
	    } break;
	case SIS1100_LOCAL_CTRL_WRITE: {
#           define d ((struct sis1100_ctrl_reg*)data)
            lockmgr(&sc->sem_hw, LK_EXCLUSIVE, 0);
	    plxwritelocal0(sc, d->offset&0x7ff, d->val);
            lockmgr(&sc->sem_hw, LK_RELEASE, 0);
            d->error=0;
#           undef d
	    } break;
	case SIS1100_REMOTE_CTRL_READ: {
#           define d ((struct sis1100_ctrl_reg*)data)
            if (sc->remote_hw==sis1100_hw_invalid) return ENXIO;
            d->error=sis1100_remote_reg_read(sc, d->offset, &d->val, 0);
#           undef d
	    } break;
	case SIS1100_REMOTE_CTRL_WRITE: {
#           define d ((struct sis1100_ctrl_reg*)data)
            if (sc->remote_hw==sis1100_hw_invalid) return ENXIO;
            d->error=sis1100_remote_reg_write(sc, d->offset, d->val, 0);
#           undef d
	    } break;
	case SIS1100_IDENT: {
#           define d ((struct sis1100_ident*)data)

	    d->local.hw_type=sc->local_ident&0xff;
	    d->local.hw_version=(sc->local_ident>>8)&0xff;
	    d->local.fw_type=(sc->local_ident>>16)&0xff;
	    d->local.fw_version=(sc->local_ident>>24)&0xff;

	    d->remote.hw_type=sc->remote_ident&0xff;
	    d->remote.hw_version=(sc->remote_ident>>8)&0xff;
	    d->remote.fw_type=(sc->remote_ident>>16)&0xff;
	    d->remote.fw_version=(sc->remote_ident>>24)&0xff;

            d->remote_ok=sc->remote_hw!=sis1100_hw_invalid;
            d->remote_online=(sis1100readreg(sc, sr)&sr_synch)==sr_synch;
#           undef d
	    } break;
/*
 * 	case SIS1100_REMOTE_RESET: {
 *             lockmgr(&sc->sem_hw, LK_EXCLUSIVE, 0);
 *             sis1100writereg(sc, cr, cr_rem_reset);
 *             sis3100writereg(sc, vme_master_sc, 8, 1);
 *             lockmgr(&sc->sem_hw, LK_RELEASE, 0);
 * 
 *             sis1100_init_remote(sc);
 * 	    } break;
 */
	case SIS1100_DEVTYPE: {
#           define d ((int*)data)
            *d=fd->subdev;
#           undef d
	    } break;
	case SIS1100_DRIVERVERSION: {
#           define d ((int*)data)
            *d=SIS1100_Version;
#           undef d
	    } break;
#if 0
        case SIS1100_BIGENDIAN: {
            int data, tmp;
            COPYIN(int);
            tmp=fd->dma_big_endian;
            if (data>=0) {
                fd->dma_big_endian=!!data;
                if (fd->dma_big_endian)
                    sis1100writereg(sc, cr, 8); /* big endian */
                else
                    sis1100writereg(sc, cr, 8<<16); /* little endian */
            }
            tmp=sis1100readreg(sc, cr);
            printf("%s: CONTROL=0x%04x\n", sc->sc_dev.dv_xname, tmp);
            data=!!(tmp&8);
            COPYOUT(int);
            } break;
#endif
        case SIS1100_MINDMALEN: {
/*
 *         0: never use DMA
 *         1: always use DMA (if size>4)
 *        >1: use DMA if transfersize (in Bytes) is >= mindmalen
 *        -1: don't change old value
 */
#           define d ((int*)data)
            int tmp[2];
            tmp[0]=fd->mindmalen_r;
            tmp[1]=fd->mindmalen_w;
            if (d[0]>-1) fd->mindmalen_r=d[0];
            if (d[1]>-1) fd->mindmalen_w=d[1];
            d[0]=tmp[0];
            d[1]=tmp[1];
#           undef d
            } break;
  	case SIS1100_SETVMESPACE: {
#           define d ((struct vmespace*)data)
            if ((d->datasize!=1) && (d->datasize!=2) && (d->datasize!=4))
                return EINVAL;
            fd->vmespace_am=d->am;
            fd->vmespace_datasize=d->datasize;
            fd->big_endian=d->swap;
            if (d->mindmalen>-1) {
                fd->mindmalen_r=d->mindmalen;
                fd->mindmalen_w=d->mindmalen;
            }
#           undef d
  	    } break;
	case SIS1100_FRONT_IO: {
#           define d ((u_int32_t*)data)
            sis1100_front_io(sc, d, 0);
#           undef d
            } break;
	case SIS1100_FRONT_PULSE: {
#           define d ((u_int32_t*)data)
            sis1100_front_pulse(sc, d, 0);
#           undef d
            } break;
	case SIS1100_FRONT_LATCH: {
#           define d ((u_int32_t*)data)
            sis1100_front_latch(sc, d, 0);
#           undef d
            } break;
	case SIS1100_LAST_ERROR: {
#           define d ((u_int32_t*)data)
            *d=fd->last_prot_err;
#           undef d
	    } break;
    	case SIS1100_MAPSIZE: {
#           define d ((u_int32_t*)data)
	    switch (fd->subdev) {
		case sis1100_subdev_remote:
		    *d=sc->rem_size;
		    break;
		case sis1100_subdev_ram:
		    *d=sc->ram_size;
		    break;
		case sis1100_subdev_ctrl:
		    *d=sc->reg_size;
		    break;
		case sis1100_subdev_dsp:
		    *d=0;
		    break;
		default:
		    return EINVAL;
	    }
#           undef d
	    } break;
#ifdef XXX
	case SIS1100_PIPE: {
#           define d ((struct sis1100_pipe*)data)
	    int res;
            if (!sc->remote_ok) return ENXIO;
    	    res=sis1100_read_pipe(sc, d);
	    if (res) return res;
	    return ENXIO;
#           undef d
	    } break;
	case SIS1100_READ_PIPE: {
	    } break;
	case SIS3100_WRITE_PIPE: {
#           define d ((struct sis1100_writepipe*)data)
            int list[192];
            if (d->num>96) return EINVAL;
            if (!sc->remote_ok) return ENXIO;
            if (copyin(d->data, list, d->num*2*sizeof(int)))
	        return EFAULT;
	    d->error=sis1100_writepipe(sc, d->am, 1/*space*/,
                d->num, d->data);
#           undef d
	    } break;
#endif
	case SIS3100_VME_PROBE: {
#           define d ((int*)data)
	    int dummy;
            if (sc->remote_hw!=sis1100_hw_vme) return ENXIO;
	    if (sis1100_tmp_read(sc, *d, fd->vmespace_am,
		    fd->vmespace_datasize, 1/*space*/, &dummy))
		return EIO;
#           undef d
	    } break;
	case SIS3100_VME_READ: {
#           define d ((struct sis1100_vme_req*)data)
            if (sc->remote_hw!=sis1100_hw_vme) return ENXIO;
	    d->error=sis1100_tmp_read(sc, d->addr, d->am, d->size,
		    1/*space*/, &d->data);
#           undef d
	    } break;
	case SIS3100_VME_WRITE: {
#           define d ((struct sis1100_vme_req*)data)
            if (sc->remote_hw!=sis1100_hw_vme) return ENXIO;
	    d->error=sis1100_tmp_write(sc, d->addr, d->am, d->size,
		    1/*space*/, d->data);
#           undef d
	    } break;
	case SIS3100_VME_BLOCK_READ: {
#           define d ((struct sis1100_vme_block_req*)data)
            int res;
            res=sis1100_block_read(sc, fd, d);
            if (res) return res;
#           undef d
	    } break;
#if XXX
	case SIS3100_VME_SUPER_BLOCK_READ: {
#           define d ((struct sis1100_vme_super_block_req*)data)
	    struct sis1100_vme_block_req reqs[10];
            ssize_t res;
            int i;
            if (sc->remote_hw!=sis1100_hw_vme) return ENXIO;
            if (copyin(d->reqs, reqs,
                    d->n*sizeof(struct sis1100_vme_block_req)))
	        return EFAULT;
            for (i=0; i<d->n; i++) {
	        res=sis1100_read_dma(sc, fd, reqs[i].addr, reqs[i].am,
	            reqs[i].size, 1/*VME-Space*/, reqs[i].fifo,
                    reqs[i].size*reqs[i].num,
                    (u_int8_t*)reqs[i].data, &reqs[i].error);
                if (res>0)
                    reqs[i].num=res/reqs[i].size;
                else
                    reqs[i].num=0;
	        if (res<0) return res;
            }
	    if (copyout(reqs, d->reqs,
                    d->n*sizeof(struct sis1100_vme_block_req)))
	        return EFAULT;
#           undef d
	    } break;
#endif
	case SIS3100_VME_BLOCK_WRITE: {
#           define d ((struct sis1100_vme_block_req*)data)
            int res;
            res=sis1100_block_write(sc, fd, d);
            if (res) return res;
#           undef d
	    } break;
        case SIS1100_FIFOMODE: {
#           define d ((int*)data)
            int tmp;
            tmp=fd->fifo_mode;
            if (*d>=0) fd->fifo_mode=!!*d;
            *d=tmp;
#           undef d
            } break;
	case SIS1100_IRQ_CTL: {
#           define d ((struct sis1100_irq_ctl*)data)
            int res;
            res=sis1100_irq_ctl(sc, fd, d);
            if (res) return res;
#           undef d
	    } break;
	case SIS1100_IRQ_GET: {
#           define d ((struct sis1100_irq_get*)data)
            int res;
            res=sis1100_irq_get(sc, fd, d);
            if (res) return res;
#           undef d
	    } break;
	case SIS1100_IRQ_ACK: {
#           define d ((struct sis1100_irq_ack*)data)
            int res;
            res=sis1100_irq_ack(sc, fd, d);
            if (res) return res;
#           undef d
	    } break;
	case SIS1100_IRQ_WAIT: {
#           define d ((struct sis1100_irq_get*)data)
            int res;
            res=sis1100_irq_wait(sc, fd, d);
            if (res) return res;
#           undef d
	    } break;
	case SIS1100_DMA_ALLOC: {
#           define d ((struct sis1100_dma_alloc*)data)
            return dma_alloc(sc, fd, d);
#           undef d
	    } break;
	case SIS1100_DMA_FREE: {
#           define d ((struct sis1100_dma_alloc*)data)
            return dma_free(sc, fd, d);
#           undef d
	    } break;
	default:
            return ENOTTY;
    }
    return 0;
}

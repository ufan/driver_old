/* $ZEL: sis1100_ioctl.c,v 1.19 2003/01/09 12:13:17 wuestner Exp $ */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <errno.h>
#include <linux/delay.h>

#include <dev/pci/sis1100_sc.h>

int
sis1100_ioctl(struct inode *inode, struct file *file,
	      unsigned int cmd, unsigned long arg)
{
    struct SIS1100_softc* sc=SIS1100SC(file);
    struct SIS1100_fdata* fd=SIS1100FD(file);

#define COPYIN(data_type) \
    do { \
        if (!(cmd&IOC_IN)) return -ENOTTY; \
        if (copy_from_user(&data, (void *)arg, sizeof(data_type))) \
	    return -EFAULT; \
    } while (0)

#define COPYOUT(data_type) \
    do { \
        if (!(cmd&IOC_OUT)) return -ENOTTY; \
        if (copy_to_user((void *)arg, &data, sizeof(data_type))) \
	    return -EFAULT; \
    } while (0)

    switch (cmd) {
	case SIS1100_LOCAL_CTRL_READ: {
	    struct sis1100_ctrl_reg data;
	    COPYIN(struct sis1100_ctrl_reg);
            down(&sc->sem_hw);
	    data.val = plxreadlocal0(sc, data.offset&0x7ff);
            up(&sc->sem_hw);
            data.error=0;
	    COPYOUT(struct sis1100_ctrl_reg);
	    } break;
	case SIS1100_LOCAL_CTRL_WRITE: {
	    struct sis1100_ctrl_reg data;
	    COPYIN(struct sis1100_ctrl_reg);
            down(&sc->sem_hw);
	    plxwritelocal0(sc, data.offset&0x7ff, data.val);
            up(&sc->sem_hw);
            data.error=0;
	    COPYOUT(struct sis1100_ctrl_reg);
	    } break;
	case SIS1100_REMOTE_CTRL_READ: {
	    struct sis1100_ctrl_reg data;
            if (sc->remote_hw==sis1100_hw_invalid) return -ENXIO;
	    COPYIN(struct sis1100_ctrl_reg);
            data.error=sis1100_remote_reg_read(sc, data.offset, &data.val, 0);
	    COPYOUT(struct sis1100_ctrl_reg);
	    } break;
	case SIS1100_REMOTE_CTRL_WRITE: {
	    struct sis1100_ctrl_reg data;
            if (sc->remote_hw==sis1100_hw_invalid) return -ENXIO;
	    COPYIN(struct sis1100_ctrl_reg);
            data.error=sis1100_remote_reg_write(sc, data.offset, data.val, 0);
	    COPYOUT(struct sis1100_ctrl_reg);
	    } break;
	case SIS1100_IDENT: {
	    struct sis1100_ident data;

	    data.local.hw_type=sc->local_ident&0xff;
	    data.local.hw_version=(sc->local_ident>>8)&0xff;
	    data.local.fw_type=(sc->local_ident>>16)&0xff;
	    data.local.fw_version=(sc->local_ident>>24)&0xff;

	    data.remote.hw_type=sc->remote_ident&0xff;
	    data.remote.hw_version=(sc->remote_ident>>8)&0xff;
	    data.remote.fw_type=(sc->remote_ident>>16)&0xff;
	    data.remote.fw_version=(sc->remote_ident>>24)&0xff;

            data.remote_ok=sc->remote_hw!=sis1100_hw_invalid;
            data.remote_online=(sis1100readreg(sc, sr)&sr_synch)==sr_synch;
	    COPYOUT(struct sis1100_ident);
	    } break;
#ifdef XXX
      case SIS1100_REMOTE_RESET: {
            down(&sc->sem_hw);
            sis1100writereg(sc, cr, cr_rem_reset);
            switch (sc->remote_hw) {
            case sis1100_hw_invalid:
                up(&sc->sem_hw);
                return -ENXIO;
            case sis1100_hw_pci:
                /* do nothing */
                break;
            case sis1100_hw_vme:
                sis3100writeremreg(sc, vme_master_sc, 8, 1);
                break;
            case sis1100_hw_camac:
                break;
            }
            up(&sc->sem_hw);
            mdelay(500);
            sis1100_init_remote(sc);
  	  } break;
#endif
	case SIS1100_DEVTYPE: {
            int data;
            data=fd->subdev;
            COPYOUT(int);
	    } break;
	case SIS1100_DRIVERVERSION: {
            int data;
            data=SIS1100_Version;
            COPYOUT(int);
	    } break;
        case SIS1100_BIGENDIAN: {
            int data[2];
            COPYIN(int[2]);
            if (data[0]>=0) {
                sis1100writereg(sc, cr, data[0]?8:8<<16);
                printk(KERN_INFO "SIS1100: tmp access set to %s endian\n",
                    data[0]?"big":"little");
            }
            if (data[1]>=0) {
                u_int32_t tmp;
                tmp=plxreadreg(sc, BIGEND_LMISC_PROT_AREA);
                if (data[1])
                    tmp|=(1<<7); /* big endian */
                else
                    tmp&=~(1<<7); /* little endian */
                plxwritereg(sc, BIGEND_LMISC_PROT_AREA, tmp);
                printk(KERN_INFO "SIS1100: dma access set to %s endian\n",
                    data[1]?"big":"little");
            }
            } break;
        case SIS1100_MINDMALEN: {
/*
 *         0: never use DMA
 *         1: always use DMA (if size>4)
 *        >1: use DMA if transfersize (in Bytes) is >= mindmalen
 *        -1: don't change old value
 */
            int data[2], tmp[2];
            COPYIN(int[2]);
            tmp[0]=fd->mindmalen_r;
            tmp[1]=fd->mindmalen_w;
            if (data[0]>=0) fd->mindmalen_r=data[0];
            if (data[1]>=0) fd->mindmalen_w=data[1];
            data[0]=tmp[0];
            data[1]=tmp[1];
            COPYOUT(int[2]);
            } break;
  	case SIS1100_SETVMESPACE: {
            struct vmespace data;
            COPYIN(struct vmespace);
            if ((data.datasize!=1) && (data.datasize!=2) && (data.datasize!=4))
                return -EINVAL;
            fd->vmespace_am=data.am;
            fd->vmespace_datasize=data.datasize;
            /*fd->big_endian=data.swap;*/
            if (data.mindmalen>=0) {
                fd->mindmalen_r=data.mindmalen;
                fd->mindmalen_w=data.mindmalen;
            }
  	    } break;
	case SIS1100_FRONT_IO: {
            u_int32_t data;
            COPYIN(u_int32_t);
            sis1100_front_io(sc, &data, 0);
            COPYOUT(u_int32_t);
            } break;
	case SIS1100_FRONT_PULSE: {
            u_int32_t data;
            COPYIN(u_int32_t);
            sis1100_front_pulse(sc, &data, 0);
            } break;
	case SIS1100_FRONT_LATCH: {
            u_int32_t data;
            COPYIN(u_int32_t);
            sis1100_front_latch(sc, &data, 0);
            COPYOUT(u_int32_t);
            } break;
	case SIS1100_LAST_ERROR: {
	    u_int32_t data;
            data=fd->last_prot_err;
	    COPYOUT(u_int32_t);
	    } break;
    	case SIS1100_MAPSIZE: {
	    u_int32_t data;
	    switch (fd->subdev) {
                case sis1100_subdev_remote:
		    data=sc->rem_size;
		    break;
		case sis1100_subdev_ram:
		    data=sc->ram_size;
		    break;
		case sis1100_subdev_ctrl:
		    data=sc->reg_size;
		    break;
		case sis1100_subdev_dsp:
		    data=0;
		    break;
		default:
		    return -EINVAL;
	    }
	    COPYOUT(u_int32_t);
	    } break;
#ifdef XXX
	case SIS1100_PIPE: {
	    struct sis1100_pipe data;
	    int res;
            if (sc->remote_hw==sis1100_hw_invalid) return -ENXIO;
	    COPYIN(struct sis1100_pipe);

    	    res=sis1100_read_pipe(sc, &data);

	    if (res) return res;
	    COPYOUT(struct sis1100_pipe);
	    } break;
	case SIS1100_READ_PIPE: {
	    } break;
	case SIS1100_WRITE_PIPE: {
	    } break;
#endif
	case SIS3100_VME_PROBE: {
	    int data, dummy;
            if (sc->remote_hw!=sis1100_hw_vme) return -ENXIO;
	    COPYIN(int);
	    if (sis1100_tmp_read(sc, data, fd->vmespace_am,
		    fd->vmespace_datasize, 1/*space*/, &dummy))
		return -EIO;
	    } break;
	case SIS3100_VME_READ: {
	    struct sis1100_vme_req data;
            if (sc->remote_hw!=sis1100_hw_vme) return -ENXIO;
	    COPYIN(struct sis1100_vme_req);
	    data.error=sis1100_tmp_read(sc, data.addr, data.am, data.size,
		    1/*space*/, &data.data);
	    COPYOUT(struct sis1100_vme_req);
	    } break;
	case SIS3100_VME_WRITE: {
	    struct sis1100_vme_req data;
            if (sc->remote_hw!=sis1100_hw_vme) return -ENXIO;
	    COPYIN(struct sis1100_vme_req);
	    data.error=sis1100_tmp_write(sc, data.addr, data.am, data.size,
		    1/*space*/, data.data);
	    COPYOUT(struct sis1100_vme_req);
	    } break;
	case SIS3100_VME_BLOCK_READ: {
	    struct sis1100_vme_block_req data;
            int res;
	    COPYIN(struct sis1100_vme_block_req);
            res=sis1100_block_read(sc, fd, &data);
	    if (res<0) return res;
	    COPYOUT(struct sis1100_vme_block_req);
	    } break;
#if XXX
	case SIS3100_VME_SUPER_BLOCK_READ: {
	    struct sis1100_vme_super_block_req data;
	    struct sis1100_vme_block_req reqs[10];
            ssize_t res;
            int i;
            if (sc->remote_hw!=sis1100_hw_vme) return -ENXIO;
	    COPYIN(struct sis1100_vme_super_block_req);
            if (copy_from_user(reqs, (void *)data.reqs,
                    data.n*sizeof(struct sis1100_vme_block_req)))
	        return -EFAULT;
            for (i=0; i<data.n; i++) {
	        res=sis1100_read_dma(fd, reqs[i].addr, reqs[i].am,
	            reqs[i].size, 1/*VME-Space*/, reqs[i].fifo,
                    reqs[i].size*reqs[i].num,
                    (u_int8_t*)reqs[i].data, &reqs[i].error);
                if (res>0)
                    reqs[i].num=res/reqs[i].size;
                else
                    reqs[i].num=0;
	        if (res<0) return res;
            }
	    if (copy_to_user((void *)data.reqs, reqs,
            data.n*sizeof(struct sis1100_vme_block_req)))
	        return -EFAULT;

	    } break;
#endif
	case SIS3100_VME_BLOCK_WRITE: {
	    struct sis1100_vme_block_req data;
            int res;
	    COPYIN(struct sis1100_vme_block_req);
            res=sis1100_block_write(sc, fd, &data);
	    if (res<0) return res;
	    COPYOUT(struct sis1100_vme_block_req);
	    } break;
        case SIS1100_FIFOMODE: {
            int data, tmp;
            COPYIN(int);
            tmp=fd->fifo_mode;
            if (data>=0) fd->fifo_mode=!!data;
            data=tmp;
            COPYOUT(int);
            } break;
	case SIS1100_IRQ_CTL: {
	    struct sis1100_irq_ctl data;
            int res;
            COPYIN(struct sis1100_irq_ctl);
            res=sis1100_irq_ctl(fd, &data);
            if (res) return res;
	    } break;
	case SIS1100_IRQ_GET: {
	    struct sis1100_irq_get data;
            int res;
            COPYIN(struct sis1100_irq_get);
            res=sis1100_irq_get(fd, &data);
            if (res) return res;
            COPYOUT(struct sis1100_irq_get);
	    } break;
	case SIS1100_IRQ_ACK: {
	    struct sis1100_irq_ack data;
            int res;
            COPYIN(struct sis1100_irq_ack);
            res=sis1100_irq_ack(fd, &data);
            if (res) return res;
	    } break;
	case SIS1100_IRQ_WAIT: {
	    struct sis1100_irq_get data;
            int res;
            COPYIN(struct sis1100_irq_get);
            res=sis1100_irq_wait(fd, &data);
            if (res) return res;
            COPYOUT(struct sis1100_irq_get);
	    } break;
        case SIS5100_CCCZ: {
            int res;
            if (sc->remote_hw!=sis1100_hw_camac) return -ENXIO;
            res=sis5100writeremreg(sc, dummy/*cr_zero*/, 0/*flag_zero*/, 0);
            if (res) return res;
            } break;
        case SIS5100_CCCC: {
            int res;
            if (sc->remote_hw!=sis1100_hw_camac) return -ENXIO;
            res=sis5100writeremreg(sc, dummy/*cr_clear*/, 0/*flag_clear*/, 0);
            if (res) return res;
            } break;
        case SIS5100_CCCI: {
            int data, res;
            if (sc->remote_hw!=sis1100_hw_camac) return -ENXIO;
	    COPYIN(int);
            res=sis5100writeremreg(sc, dummy/*cr_inhibit*/, data, 0);
            if (res) return res;
            } break;
        case SIS5100_CNAF: {
            struct sis1100_camac_req data;
            COPYIN(struct sis1100_camac_req);
            return -ENXIO;
            } break;
	default:
            return -ENOTTY;
    }
    return 0;
}

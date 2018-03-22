/* $ZEL: sis1100_read.c,v 1.12 2003/01/09 12:13:18 wuestner Exp $ */

#include "Copyright"

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

static int
check_access(loff_t ppos, loff_t maxsize, size_t count, int datasize,
        const char* buf)
{
    if (ppos>=maxsize) {
    	printk(KERN_INFO "sis1100_r/w: start addr out of range\n");
    	return -EINVAL;
    }

    if ((ppos+count>maxsize)||(ppos+count<ppos)) {
    	printk(KERN_INFO "sis1100_r/w: end addr out of range\n");
    	return -EINVAL;
    }
    
    if ((ppos|(off_t)buf|count) & (datasize-1)) {
    /* == (ppos&(datasize-1))||(buf&(datasize-1))...*/
    	printk(KERN_INFO "sis1100_r/w: unaligned access\n");
    	return -EINVAL;
    }
    return 0;
}

ssize_t sis1100_read(struct file* file, char* buf, size_t count,
    loff_t* ppos)
{
    struct SIS1100_softc* sc=SIS1100SC(file);
    struct SIS1100_fdata* fd=SIS1100FD(file);
    int res;
    int32_t am;
    u_int32_t datasize;
    int space, fifo;
    loff_t maxsize;

    /*printk(KERN_ERR "read pos=%lld count=%d data=%p\n",
                *ppos, count, buf);*/
    if (sc->remote_hw==sis1100_hw_invalid) return -ENXIO;
    if (!count) return 0;

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

    if (check_access(*ppos, maxsize, count, datasize, buf))
        return -EINVAL;
    if (!access_ok(VERIFY_WRITE, buf, count))
        return -EFAULT;

    if (count==datasize) {
        u_int32_t var;
        if ((res=sis1100_tmp_read(sc, *ppos, am, datasize, space, &var))!=0) {
            fd->last_prot_err=res;
            res=-EIO;
        } else {
            switch (datasize) {
	    case 4: __put_user(var, (u_int32_t*)buf); break;
	    case 2: __put_user(var, (u_int16_t*)buf); break;
	    case 1: __put_user(var, (u_int8_t*)buf); break;
            }
            res=count;
        }
    } else {
        if ((datasize==4) && fd->mindmalen_r && (count>=fd->mindmalen_r)) {
            res=sis1100_read_dma(fd, *ppos, am, datasize,
                    space, fifo, count, buf, &fd->last_prot_err);
            if (fd->last_prot_err && ((fd->last_prot_err&0x200)!=0x200))
                    res=-EIO;
        } else {
            res=sis1100_read_loop(fd, *ppos, am, datasize,
                    space, fifo, count, buf, &fd->last_prot_err);
        }
    }

    if (res>=0) *ppos+=res;
    return res;
}

ssize_t sis1100_write(struct file* file, const char* buf, size_t count,
    loff_t* ppos)
{
    struct SIS1100_softc* sc=SIS1100SC(file);
    struct SIS1100_fdata* fd=SIS1100FD(file);
    int res;
    int32_t am;
    u_int32_t datasize;
    int space, fifo;
    loff_t maxsize;

    /*printk(KERN_ERR "write pos=%lld count=%d data=%p\n",
                *ppos, count, buf);*/
    if (sc->remote_hw==sis1100_hw_invalid) return -ENXIO;
    if (!count) return 0;

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

    if (check_access(*ppos, maxsize, count, datasize, buf))
        return -EINVAL;
    if (!access_ok(VERIFY_READ, buf, count))
        return -EFAULT;

    if (count==datasize) {
        u_int32_t data;

        switch (datasize) {
        case 4: __get_user(data, (u_int32_t*)buf); break;
        case 2: __get_user(data, (u_int16_t*)buf); break;
        case 1: __get_user(data, (u_int8_t*)buf); break;
        default: data=0;
        }
        if ((res=sis1100_tmp_write(sc, *ppos, am, datasize,
                space, data))!=0) {
            fd->last_prot_err=res;
            res=-EIO;
        } else
            res=count;
    } else {
        if ((datasize==4) && fd->mindmalen_w && (count>=fd->mindmalen_w)) {
            res=sis1100_write_dma(fd, *ppos, am, datasize,
                    space, fifo, count, buf, &fd->last_prot_err);
        } else {
            res=sis1100_write_loop(fd, *ppos, am, datasize,
                    space, fifo, count, buf, &fd->last_prot_err);
        }
    }

    if (res>=0) *ppos+=res;
    return res;
}

/* SEEK_... normally defined in stdio.h, fcntl.h and unistd.h */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

loff_t sis1100_llseek(struct file* file, loff_t offset, int orig)
{
    struct SIS1100_softc* sc=SIS1100SC(file);
    struct SIS1100_fdata* fd=SIS1100FD(file);
    loff_t old=file->f_pos;

    switch (orig) {
        case SEEK_SET: file->f_pos=offset; break;
        case SEEK_CUR: file->f_pos+=offset; break;
        case SEEK_END:
            if (fd->subdev==sis1100_subdev_ram) {
                file->f_pos=sc->ram_size+offset;
            } else
                return -EINVAL;
            break;
    }
    if ((file->f_pos<0) ||
        (file->f_pos>
            ((fd->subdev==sis1100_subdev_ram)?sc->ram_size:0xffffffffU))) {
        file->f_pos=old;
        return -EINVAL;
    }
    return file->f_pos;
}

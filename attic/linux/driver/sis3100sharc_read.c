/* $ZEL: sis3100sharc_read.c,v 1.5 2002/08/12 10:36:41 wuestner Exp $ */

#include "Copyright"

#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/wrapper.h>
#include <linux/pci.h>
#include <asm/uaccess.h>

#include <dev/pci/sis1100_sc.h>

static int
check_range(struct SIS1100_softc* sc, const char* buf, size_t count, loff_t pos)
{
#if 0
    /* start addr out of range? */
    
    if ((pos<0) || (pos>=sc->sharc_size)) {
    	printk(KERN_INFO "sis3100sh_r/w: start addr out of range\n");
    	return -EINVAL;
    }
    /* end addr out of range? */
    if ((pos+count>sc->sharc_size)||(pos+count<pos)) {
    	printk(KERN_INFO "sis3100sh_r/w: end addr out of range\n");
    	return -EINVAL;
    }
#endif
    return 0;
}

ssize_t sis3100sharc_read(struct file* file, char* buf, size_t count,
    loff_t* ppos)
{
    struct SIS1100_softc* sc=SIS1100SC(file);
    struct SIS1100_fdata* fd=SIS1100FD(file);
    int res;

    if (!sc->remote_ok) return -ENXIO;
    if ((res=check_range(sc, buf, count, *ppos)<0)) return res;

    if (count==4) {
        if(sis1100_tmp_read(sc, *ppos, -1/*am*/, 4/*datasize*/, 6/*space*/,
                buf, 1)!=0)
            res=-EIO;
        else
            res=count;
    } else {
        printk(KERN_INFO "calling sis1100_read_dma\n");
        res=sis1100_read_dma(fd, *ppos, -1/*am*/, 4/*datasize*/, 6/*space*/,
            0, count, buf, &fd->last_prot_err);
    }

    if (res<0)
	return res;
    else {
	*ppos+=res;
	return res;
    }
}

ssize_t sis3100sharc_write(struct file* file, const char* buf, size_t count,
    loff_t* ppos)
{
    struct SIS1100_softc* sc=SIS1100SC(file);
    struct SIS1100_fdata* fd=SIS1100FD(file);
    int res;

    if (!sc->remote_ok) return -ENXIO;
    if ((res=check_range(sc, buf, count, *ppos))<0) return res;


    if (count==4) {
        if (sis1100_tmp_write(sc, *ppos, -1/*am*/, 4/*datasize*/,
                6/*space*/, *(u_int32_t*)buf)!=0)
            res=-EIO;
        else
            res=count;
    } else {
        res=sis1100_write_dma(fd, *ppos, -1/*am*/,
                4/*datasize*/, 6/*space*/, 0, count, buf, &fd->last_prot_err);
    }

    if (res<0)
	return res;
    else {
	*ppos+=res;
	return res;
    }
}

/* SEEK_... normally defined in stdio.h, fcntl.h and unistd.h */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

loff_t sis3100sharc_llseek(struct file* file, loff_t offset, int orig)
{
    struct SIS1100_softc* sc=SIS1100SC(file);
    /*loff_t old=file->f_pos;*/
/*
    printk(KERN_INFO "sis3100sharc_seek: offset=%Ld, orig=%d\n", offset, orig);
*/
    switch (orig) {
    	case SEEK_SET: file->f_pos=offset; break;
    	case SEEK_CUR: file->f_pos+=offset; break;
    	case SEEK_END:
	    file->f_pos=sc->sharc_size+offset;
	    break;
    }
#if 0
    if ((file->f_pos<0) || (file->f_pos>sc->sharc_size)) {
        file->f_pos=old;
    	return -EINVAL;
    }
#endif
    return file->f_pos;
}

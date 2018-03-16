#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "dev/pci/sis1100_var.h"

static int write_reg(int p, int offset, u_int32_t value)
{
        struct sis1100_ctrl_reg reg;
        reg.offset=offset;
        reg.val=value;
        if (ioctl(p, SIS1100_CONTROL_WRITE, &reg)<0) {
                printf("SIS1100_CONTROL_WRITE 0x%08x -> 0x%03x: %s\n",
                        reg.val, reg.offset, strerror(errno));
                return -1;
        }
        if (reg.error) {
                printf("SIS1100_CONTROL_WRITE 0x%08x -> 0x%03x: error=%d\n",
                        reg.val, reg.offset, reg.error);
                return -1;
        }
        return 0;
}

/****************************************************************************/
static u_int32_t* mmap_vme_space(int p, size_t* mapped_len)
{
    u_int32_t *space, size;

    if (ioctl(p, SIS1100_MAPSIZE, &size)) {
        printf("ioctl(SIS1100_MAPSIZE): %s\n", strerror(errno));
        return 0;
    }

    /* map the vme space */
    space=mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, p, 0);
    if (space==MAP_FAILED)
        {
        printf("mmap vme space: %s\n", strerror(errno));
        return 0;
        }
    *mapped_len=size;
    return space;
}
/****************************************************************************/
int main(int argc, char* argv[])
{
        int p;
        volatile u_int32_t* vmespace;
        volatile u_int32_t data;
        size_t vmespacelen;

        if (argc!=2) {
                printf("usage: %s path\n", argv[0]);
                return 1;
        }

        if ((p=open(argv[1], O_RDWR, 0))<0) {
                printf("open \"%s\": %s\n", argv[1], strerror(errno));
                return 1;
        }

        vmespace=mmap_vme_space(p, &vmespacelen);
        if (!vmespace) {
                printf("map vme failed\n");
                return 1;
        }
        printf("vmespacelen=%d MByte\n", vmespacelen>>20);

        if (write_reg(p, 0x400, 0xff01081c)<0) return 1;
        if (write_reg(p, 0x404, 0x09)<0) return 1;
        if (write_reg(p, 0x40c, 0x0)<0) return 1;

        if (write_reg(p, 0x408, 0xffffffff)<0) return 1;
        data=vmespace[0];
        if (write_reg(p, 0x408, 0xfffffff0)<0) return 1;
        data=vmespace[0];
        if (write_reg(p, 0x408, 0xffffff00)<0) return 1;
        data=vmespace[0];
        if (write_reg(p, 0x408, 0xfffff000)<0) return 1;
        data=vmespace[0];
        if (write_reg(p, 0x408, 0xffff0000)<0) return 1;
        data=vmespace[0];
        if (write_reg(p, 0x408, 0xfff00000)<0) return 1;
        data=vmespace[0];
        if (write_reg(p, 0x408, 0xff000000)<0) return 1;
        data=vmespace[0];
        if (write_reg(p, 0x408, 0xf0000000)<0) return 1;
        data=vmespace[0];
        if (write_reg(p, 0x408, 0x00000000)<0) return 1;
        data=vmespace[0];

        if (write_reg(p, 0x408, 0x00000000)<0) return 1;
        data=vmespace[0x1];
        data=vmespace[0x2];
        data=vmespace[0x4];
        data=vmespace[0x8];
        data=vmespace[0x10];
        data=vmespace[0x20];
        data=vmespace[0x40];
        data=vmespace[0x80];
        data=vmespace[0x100];
        data=vmespace[0x200];
        data=vmespace[0x400];
        data=vmespace[0x800];
        data=vmespace[0x1000];
        data=vmespace[0x2000];
        data=vmespace[0x4000];
        data=vmespace[0x8000];
        data=vmespace[0x10000];
        data=vmespace[0x20000];
        data=vmespace[0x40000];
        data=vmespace[0x80000];
        data=vmespace[0x100000];
        data=vmespace[0x200000];
        data=vmespace[0x400000];
        data=vmespace[0x800000];


/*
 *         if (write_reg(p, 0x400, 0x0f01101c)<0) return 1;
 *         if (write_reg(p, 0x404, 0x09)<0) return 1;
 *         if (write_reg(p, 0x404, 0x0)<0) return 1;
 *         if (write_reg(p, 0x404, 0x0)<0) return 1;
 * 
 *         data=vmespace[0xffffffff];
 */
        
        if (munmap((void*)vmespace, vmespacelen)<0) {
                fprintf(stderr, "munmap: %s\n", strerror(errno));
        }
        close(p);

        return 0;
}

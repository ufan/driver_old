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

int main(int argc, char* argv[])
{
    int p;
    struct sis1100_ident ident;
    struct sis1100_ctrl_reg reg;
    volatile u_int32_t* regspace;
    u_int32_t regsize;
    u_int32_t data;
    enum sis1100_subdev devtype;

    if (argc!=2)
        {
        fprintf(stderr, "usage: %s path\n", argv[0]);
        return 1;
        }

    if ((p=open(argv[1], O_RDWR, 0))<0) {
        fprintf(stderr, "open \"%s\": %s\n", argv[1], strerror(errno));
        return 1;
    }

    if (ioctl(p, SIS1100_IDENT, &ident)<0) {
        fprintf(stderr, "ioctl(SIS1100_IDENT): %s\n", strerror(errno));
        return 2;
    }
    printf("local:\n");
    printf("  hw_type   : %d\n",   ident.local.hw_type);
    printf("  hw_version: %d\n",   ident.local.hw_version);
    printf("  fw_type   : %d\n",   ident.local.fw_type);
    printf("  fw_version: %d\n\n", ident.local.fw_version);
    printf("remote:\n");
    printf("  hw_type   : %d\n",   ident.remote.hw_type);
    printf("  hw_version: %d\n",   ident.remote.hw_version);
    printf("  fw_type   : %d\n",   ident.remote.fw_type);
    printf("  fw_version: %d\n\n", ident.remote.fw_version);

    printf("  remote side is %s and %svalid\n",
        ident.remote_online?"online":"offline",
        ident.remote_ok?"":"not ");

    if ((ident.local.hw_type!=1)||(ident.local.hw_version!=1)||
        (ident.local.fw_type!=1)) {
        fprintf(stderr, "unsupported bord version\n");
        return 2;
    }

    reg.offset=0;
    if (ioctl(p, SIS1100_CONTROL_READ, &reg)<0) {
        fprintf(stderr, "ioctl(SIS1100_CONTROL_READ, offs=0): %s\n",
            strerror(errno));
        return 2;
    }
    if (reg.error!=0) {
        fprintf(stderr, "ioctl(SIS1100_CONTROL_READ, offs=0): error=0x%x\n",
            reg.error);
        return 2;
    }
    if (reg.val!=(ident.local.hw_type|
        (ident.local.hw_version<<8)|
        (ident.local.fw_type<<16)|
        (ident.local.fw_version<<24))) {
        fprintf(stderr, "local ident 0x%08x does not match SIS1100_IDENT\n",
            reg.val);
        return 2;
    }

    if (ioctl(p, SIS1100_DEVTYPE, &devtype)<0) {
        printf("ioctl(SIS1100_DEVTYPE): %s\n", strerror(errno));
        return 1;
    }
    if (devtype!=sis1100_subdev_ctrl) {
        printf("You have to use the control device (sis1100ctrl_xx).\n");
        return 1;
    }

    if (ioctl(p, SIS1100_MAPSIZE, &regsize)<0) {
        fprintf(stderr, "ioctl(SIS1100_MAPSIZE 1): %s\n", strerror(errno));
        return 1;
    }
    printf("size of regspace=0x%x\n", regsize);

    regspace=mmap(0, regsize, PROT_READ|PROT_WRITE, MAP_SHARED, p, 0);
    if (regspace==MAP_FAILED) {
        fprintf(stderr, "mmap register space: %s\n", strerror(errno));
        return 2;
    }

    data=regspace[0];
    if (data!=reg.val) {
        fprintf(stderr, "local ident 0x%08x (mapped) does not match SIS1100_IDENT\n",
            data);
        return 2;
    }

    if (munmap((void*)regspace, regsize)<0) {
        fprintf(stderr, "munmap: %s\n", strerror(errno));
        return 2;
    }

    if (close(p)<0) {
        fprintf(stderr, "close: %s\n", strerror(errno));
        return 2;
    }
    return 0;
}

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
    
    if ((ident.local.hw_type!=1)||(ident.local.hw_version!=1)||
        (ident.local.fw_type!=1)) {
        fprintf(stderr, "unsupported bord version:\n");
        fprintf(stderr, "  hw_type   : %d\n",   ident.local.hw_type);
        fprintf(stderr, "  hw_version: %d\n",   ident.local.hw_version);
        fprintf(stderr, "  fw_type   : %d\n",   ident.local.fw_type);
        fprintf(stderr, "  fw_version: %d\n\n", ident.local.fw_version);
        return 2;
    }




    if (close(p)<0) {
        fprintf(stderr, "close: %s\n", strerror(errno));
        return 2;
    }
    return 0;
}

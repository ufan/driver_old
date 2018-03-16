#define _GNU_SOURCE
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "dev/pci/sis1100_var.h"

static int do_write(int p, int start, int size, int* data)
{
    off_t pos;
    int res;

    pos=lseek(p, sizeof(int)*start, SEEK_SET);
    if (pos==(off_t)-1) {
        perror("do_write::lseek");
        return 1;
    }
    res=write(p, data, size*sizeof(int));
    if (res!=size*sizeof(int)) {
        u_int32_t error;
        ioctl(p, SIS1100_LAST_ERROR, &error);
        if (res<0) {
            fprintf(stderr, "write: %s; error=0x%x\n", strerror(errno), error);
        } else {
            fprintf(stderr, "write: res=%d; error=0x%x\n", res, error);
        }
        return 1;
    }
    return 0;
}

static int do_read(int p, int start, int size, int* data)
{
    off_t pos;
    int res;

    pos=lseek(p, sizeof(int)*start, SEEK_SET);
    if (pos==(off_t)-1) {
        perror("do_read::lseek");
        return 1;
    }
    res=read(p, data, size*sizeof(int));
    if (res!=size*sizeof(int)) {
        u_int32_t error;
        ioctl(p, SIS1100_LAST_ERROR, &error);
        if (res<0) {
            fprintf(stderr, "read: %s; error=0x%x\n", strerror(errno), error);
        } else {
            fprintf(stderr, "read: res=%d; error=0x%x\n", res, error);
        }
        return 1;
    }
    return 0;
}

static void fill_buf(int size, int* buf, int num)
{
    int i;
    for (i=0; i<size; i++) {
        buf[i]=num<<20|i;
    }
}

static int test_buf(int size, int* obuf, int* ibuf)
{
    int i, n=0;

    for (i=0; i<size; i++) {
        if (obuf[i]!=ibuf[i]) {
            if (n++<20) {
                printf("[%3d] 0x%08X --> 0x%08X\n", i, obuf[i], ibuf[i]);
            }
        }
    }
    if (n) printf("      %d errors\n", n);
    return n;
}

int main(int argc, char* argv[])
{
    int *ibuf, *obuf;
    off_t max;
    int p, size;

    if (argc!=2) {
        fprintf(stderr, "usage: %s path\n", argv[0]);
        return 1;
    }

    if ((p=open(argv[1], O_RDWR, 0))<0) {
        perror("open");
        return 1;
    }

    max=lseek(p, 0, SEEK_END);
    if (max==(off_t)-1) {
        perror("lseek(0, SEEK_END)");
        return 1;
    }
    fprintf(stderr, "size of sdram is %08Lx (%Ld MByte)\n", max, max/(1<<20));

    max/=sizeof(int);
    ibuf=calloc(max, sizeof(int));
    obuf=calloc(max, sizeof(int));
    if (!ibuf || !obuf) {
        perror("calloc");
        return 1;
    }

    printf("calloc ok\n");

    for (size=256; size<=16384; size+=256) {
        printf("size=%d\n", size);
        fill_buf(size, obuf, size);
        do_write(p, 0, size, obuf);
        do_read(p, 0, size, ibuf);
        test_buf(size, obuf, ibuf);
    }

    free(obuf);
    free(ibuf);
    close(p);

    return 0;
}

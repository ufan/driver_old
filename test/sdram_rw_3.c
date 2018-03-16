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

static int generate_size(int max)
{
    int maxbits, bits;
    int mask, size;

    maxbits=0;
    while (1<<maxbits<max) maxbits++;
    bits=random()%maxbits+1;
    mask=0xffffffff>>(32-bits);
    size=random()%mask+1;
    if ((size>max) || (size==0)) {
        printf("invalid size %d\n", size);
        printf("  max    =0x%08x\n", max);
        printf("  maxbits=%d\n", maxbits);
        printf("  bits   =%d\n", bits);
        printf("  mask   =0x%08x\n", mask);
        size=1;
    }
    return size;
}

static int generate_start(int max, int size)
{
    int space, offs;
    space=max-size;
    offs=random()%(space+1);
    return offs;
}

/*
 * static void fill_buf(int size, int* buf)
 * {
 *     int i;
 *     for (i=0; i<size; i++) {
 *         buf[i]=random();
 *     }
 * }
 */

static void fill_buf(int size, int* buf, int num)
{
    int i;
    for (i=0; i<size; i++) {
        buf[i]=num<<28|i;
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

int main(int argc, char* argv[])
{
    int *ibuf, *obuf;
    off_t max;
    int p, size, offs, num;

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
    srandom(17);

    if (do_write(p, 0, max, obuf)) {
        printf("initial write failed\n");
        return 1;
    }

    num=0;
    while (++num) {
        size=generate_size(16384);
        /*printf("size=%d\n", size);*/
        offs=generate_start(max, size);
        if (offs+size>max) {
            printf("invalid offs: size=%d offs=%d\n", size, offs);
        }
        /*printf("  offs=%d\n", offs);*/

        printf("write %08X words from %08X; (%d)\n", size, offs, num);
        fill_buf(size, obuf+offs, num);
        if (do_write(p, offs, size, obuf+offs)) {
            printf("write failed\n");
            return 1;
        }

        printf("read  %08X words from %08X\n", size, offs);
        if (do_read(p, offs, size, ibuf+offs)) {
            printf("read failed\n");
            return 1;
        }
        /*if (test_buf(size, obuf+offs, ibuf+offs)) return 1;*/
        test_buf(size, obuf+offs, ibuf+offs);

    }

    free(obuf);
    free(ibuf);
    close(p);

    return 0;
}

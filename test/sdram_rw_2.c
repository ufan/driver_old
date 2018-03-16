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

int main(int argc, char* argv[])
{
    int *ibuf, *obuf;
    off_t max;
    int p;
    int k;

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

    if (do_write(p, 0, max, obuf)) {
        printf("initial write failed\n");
        return 1;
    }

    for (k=0; k<8; k++) {
        unsigned int w;
        int res;

        w=1<<k;
        memset(obuf, w, max*sizeof(int));
        printf("write %2x\n", w);
        if (do_write(p, 0, max, obuf)) {
            printf("write failed\n");
            return 1;
        }
        printf("read  %2x\n", w);
        if (do_read(p, 0, max, ibuf)) {
            printf("read write failed\n");
            return 1;
        }
        res=bcmp(obuf, ibuf, max*sizeof(int));
        if (res) {printf("bcmp failed\n");}
    }


    free(obuf);
    free(ibuf);
    close(p);

    return 0;
}

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

/****************************************************************************/
static int* alloc_buf(int num)
{
    unsigned long int p;
    int pagesize, pagemask;

    pagesize=getpagesize();
    pagemask=pagesize-1;
    p=(unsigned long int)calloc(num+pagesize, 1);
    if (!p) return (int*)p;
    p=(p+pagesize-1)&(~pagemask);
    return (int*)p;
}

static int generate_size(int max)
{
    int maxbits, bits;
    int mask, size;

    maxbits=0;
    while (1<<maxbits<max) maxbits++;
    bits=random()%maxbits+1;
    mask=0xffffffff>>(32-bits);
    size=random()%mask+1;
    return size;
}

static int generate_start(int max, int size)
{
    int space, offs;
    space=max-size;
    offs=random()%(space+1);
    return offs;
}

static void check_it(int p, int size, int* ob, int* ib, int mem, int err_addr)
{
    u_int32_t data;
    int l;

    lseek(p, sizeof(int)*(mem+err_addr), SEEK_SET);
    read(p, &data, sizeof(int));
    printf("/524288=%d %%524288=%d diff to end=%d\n",
        ((err_addr+1)*4)/524288,
        ((err_addr+1)*4)%524288,
        size-err_addr);
/*
    printf("** ob: %08X mem: %08X ib: %08X\n", ob[err_addr], data, ib[err_addr]);
    printf("** ob+offs=%08X ib+offs=%08X\n",
        (unsigned int)(ob+err_addr), (unsigned int)(ib+err_addr));
*/
    for (l=0; l<5; l++) {
        lseek(p, sizeof(int)*mem, SEEK_SET);
        write(p, ob, size*sizeof(int));
        lseek(p, sizeof(int)*mem, SEEK_SET);
        read(p, ib, size*sizeof(int));
        if (bcmp(ib, ob, size*sizeof(int))) {
            int i, n;
            printf("## size=%08X ob=%08lX ib=%08lX mem=%08lX\n", size,
                (unsigned long)ob, (unsigned long)ib, (unsigned long)mem);
            for (i=0, n=0; (i<size) && (n<20); i++) {
                if (ib[i]!=ob[i]) {
                    printf("## [%08X] %08X %08X\n", i, ob[i], ib[i]);
                    n++;
                }
            }
        }
    }
}

int main(int argc, char* argv[])
{
    int *ibuf, *obuf;
    off_t max;
    int p, i, n, size;

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

    ibuf=alloc_buf(max*sizeof(int));
    printf("ibuf=%08X\n", (unsigned int)ibuf);
    obuf=alloc_buf(max*sizeof(int));
    printf("obuf=%08X\n", (unsigned int)obuf);
    if (!ibuf || !obuf) {
        perror("calloc");
        return 1;
    }
size=max;
    while (1) {
        int res;
        int ob_start, ib_start, mem_start;
        int *ib, *ob;
        off_t pos;

        /*size=generate_size(max);*/
        ob_start=generate_start(max, size);
        ib_start=generate_start(max, size);
        mem_start=generate_start(max, size);
        /*printf("%8d %8d %8d %8d\n", size, ob_start, ib_start, mem_start);*/
        /*printf("."); fflush(stdout);*/
        ob=obuf+ob_start;
        for (i=0; i<size; i++) *ob++=random();

        pos=lseek(p, sizeof(int)*mem_start, SEEK_SET);
        if (pos==(off_t)-1) {
            perror("lseek");
            return -1;
        }

        ob=obuf+ob_start;
        res=write(p, ob, size*sizeof(int));
        if (res!=size*sizeof(int)) {
            u_int32_t error;
            ioctl(p, SIS1100_LAST_ERROR, &error);
            if (res<0) {
                fprintf(stderr, "write: %s; error=0x%x\n", strerror(errno), error);
            } else {
                fprintf(stderr, "write: res=%d; error=0x%x\n", res, error);
            }
            return -1;
        }

        pos=lseek(p, sizeof(int)*mem_start, SEEK_SET);
        if (pos==(off_t)-1) {
            perror("lseek");
            return -1;
        }

        ib=ibuf+ib_start;
        res=read(p, ib, size*sizeof(int));
        if (res!=size*sizeof(int)) {
            u_int32_t error;
            ioctl(p, SIS1100_LAST_ERROR, &error);
            if (res<0) {
                fprintf(stderr, "read: %s; error=0x%x\n", strerror(errno), error);
            } else {
                fprintf(stderr, "read: res=%d; error=0x%x\n", res, error);
            }
            return -1;
        }

        if (bcmp(ib, ob, size*sizeof(int))) {
            int err_addr;

            printf("\nsize=%08X ob=%08lX ib=%08lX mem=%08lX\n", size,
                (unsigned long)ob, (unsigned long)ib, (unsigned long)mem_start);
            err_addr=-1;
            for (i=0, n=0; (i<size) && (n<20); i++) {
                if (ib[i]!=ob[i]) {
                    printf("[%08X] %08X %08X\n", i, ob[i], ib[i]);
                    if (err_addr==-1) err_addr=i;
                    n++;
                }
            }
            check_it(p, size, ob, ib, mem_start, err_addr);
            size=size/10;
            printf("new size=%d\n", size);
        }
    }

    close(p);

    return 0;
}

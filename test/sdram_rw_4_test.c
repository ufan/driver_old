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
    printf("pagesize=%d\n", pagesize);
    p=(unsigned long int)calloc(num+pagesize, 1);
    if (!p) return (int*)p;
    printf("p_0=0x%08lx\n", p);
    p=(p+pagesize-1)&(~pagemask);
    printf("p_1=0x%08lx\n", p);
    return (int*)p;
}
/****************************************************************************/
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
/****************************************************************************/
static int test_buf(int size, int* ibuf)
{
    int i, n, res, state;

    n=0; state=0;
    for (i=0; (i<size)&&(n<5); i+=4) {
        res=(ibuf[i]!=0x12345678)||(ibuf[i+1]!=i);
        if (res!=state) {
            int start, j;
            printf("\n~~~~~~~~~~~~~~~~~~~~~~~%d~~~~~~~~~~~~~~~~~~~~~~~\n", res);
            start=i-20; if (start<0) start=0;
            for (j=start; j<start+40; j+=4)
                printf("[%08X]: %08X %08X %08X %08X\n",
                    j, ibuf[j], ibuf[j+1], ibuf[j+2], ibuf[j+3]);
            state=res;
            n++;
        }
    }
    return n;
}
/****************************************************************************/
int main(int argc, char* argv[])
{
    int *ibuf;
    off_t max;
    int p;

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
    if (!ibuf) {
        perror("calloc");
        return 1;
    }

    if (do_read(p, 0, max, ibuf)) {
        printf("\nread failed\n");
        return 1;
    }

    test_buf(max, ibuf);
        
    close(p);

    return 0;
}
/****************************************************************************/
/****************************************************************************/

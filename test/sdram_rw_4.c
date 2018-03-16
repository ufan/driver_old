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
#if 0
static void light_set(int p, int v)
{
    u_int32_t reg, set, res;

    set=((v&1)<<7)|((v&6)<<9);
    res=~(set<<16)&0x0c800000;
    reg=set|res;
    ioctl(p, SIS1100_FRONT_IO, &reg);
}
#endif
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
#if 0
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
#endif

static int generate_start(int max, int size)
{
    int space, offs;
    space=max-size;
    offs=random()%(space+1);
    return offs;
}

static int test_obuf(int size, int* obuf)
{
    int i, n;

    n=0;
    for (i=0; i<size; i+=4) {
        if ((obuf[i]!=0x12345678)||(obuf[i+1]!=i)) {
            if (!n) printf("\nobuf corrupted:\n");
            if (n<20)
                printf("[%08X]: %08X %08X %08X %08X\n",
                    i, obuf[i], obuf[i+1], obuf[i+2], obuf[i+3]);
            n++;
        }
    }
    return n;
}

static int test_buf(int size, int* obuf, int* ibuf)
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

static int test_write(int p, int start, int size, int* data)
{
    off_t pos;
    int res, word1, word2;
    
    pos=lseek(p, sizeof(int)*(start+2), SEEK_SET);
    if (pos==(off_t)-1) {
        perror("test_write::lseek a");
        return 1;
    }
    res=read(p, &word1, sizeof(int));
    if (res!=sizeof(int)) {
        perror("test_write a");
        return 1;
    }
    pos=lseek(p, sizeof(int)*(start+size-2), SEEK_SET);
    if (pos==(off_t)-1) {
        perror("test_write::lseek b");
        return 1;
    }
    res=read(p, &word2, sizeof(int));
    if (res!=sizeof(int)) {
        perror("test_write b");
        return 1;
    }
    if ((word1!=start)||(word2!=start)) {
        printf("test_write: start=%08X\n", start);
        printf("test_write: [%08X]: %08X-->%08X\n", start+2, data[2], word1);
        printf("            [%08X]: %08X-->%08X\n", start+size-2,
                data[size-2], word2);
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

static void prepare_data(int* buf, int offs, int size)
{
    int* start=buf+offs;
    int i;

    for (i=0; i<size; i+=4) {
        start[i+2]=offs;
        start[i+3]=size;
    }
}

int main(int argc, char* argv[])
{
    int *ibuf, *obuf;
    off_t max;
    int p, size, offs, num, i, j;

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
    obuf=alloc_buf(max*sizeof(int));
    if (!ibuf || !obuf) {
        perror("calloc");
        return 1;
    }

    for (i=0; i<max; i+=4) {
        obuf[i]=0x12345678;
        obuf[i+1]=i;
    }

    if (do_write(p, 0, max, obuf)) {
        printf("initial write failed\n");
        return 1;
    }
    if (test_obuf(max, obuf)) {
        printf("initial obuf test failed\n");
    }
    srandom(17);
    num=0;
    while (++num) {
        int res;

        for (j=0; j<1000; j++) {
            size=/*generate_size(max)*/ 16384;
            size=(size+3)&~3;

            /*printf("size=%d\n", size);*/
            offs=generate_start(max, size);
            offs&=~3;
            if (offs+size>max) {
                printf("\ninvalid offs: size=%d offs=%d\n", size, offs);
            }
            /*printf("  offs=%d\n", offs);*/

            /*printf("write %08X words from %08X; (%d)\n", size, offs, num);*/
            /*printf("+"); fflush(stdout);*/
            prepare_data(obuf, offs, size);
            if (do_write(p, offs, size, obuf+offs)) {
                printf("\nwrite failed\n");
                return 1;
            }
            if (test_write(p, offs, size, obuf+offs)) {
                printf("test_write: Fehler\n");
                return 1;
            }
        }

        if (do_read(p, 0, max, ibuf)) {
            printf("\nread failed\n");
            return 1;
        }

        printf("-"); fflush(stdout);
        res=test_buf(max, obuf, ibuf);
        if (res) {
            ioctl(p, SIS1100_DUMP);
            return 0;
        }
        
    }

    close(p);

    return 0;
}

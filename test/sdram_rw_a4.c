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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "dev/pci/sis1100_var.h"

#define VMESTART 0x84000000

int sis1100;
off_t devstart;
size_t maxsize;
int* hbuf;

static void
printusage(int argc, char* argv[])
{
    printf("usage: %s"
        " [-s maxsize] sis1100_path\n",
        argv[0]);
}

static int
getoptions(int argc, char* argv[])
{
    extern char *optarg;
    extern int optind;
    extern int opterr;
    extern int optopt;
    int errflag, c;
    char* sis1100_path=0;
    const char* args="s:";

    optarg=0; errflag=0;
    
    while (!errflag && ((c=getopt(argc, argv, args))!=-1)) {
        switch (c) {
        case 's': maxsize=atoi(optarg); break;
        default: errflag=1;
        }
    }

    if (errflag || optind!=argc-1) {
        printusage(argc, argv);
        return -1;
    }

    sis1100_path=argv[optind];
    if ((sis1100=open(sis1100_path, O_RDWR, 0))<0) {
        printf("open \"%s\": %s\n", sis1100_path, strerror(errno));
        return -1;
    }

    return 0;
}

static int do_write(int p, int start, int size, int* data)
{
    off_t pos;
    int res;

    pos=lseek(p, sizeof(int)*start+devstart, SEEK_SET);
    if (pos==(off_t)-1) {
        perror("do_write::lseek");
        return 1;
    }
    res=write(p, data, size*sizeof(int));
    if (res!=size*sizeof(int)) {
        u_int32_t error;
        ioctl(p, SIS1100_LAST_ERROR, &error);
        if (res<0) {
            printf("write: %s; error=0x%x\n", strerror(errno), error);
        } else {
            printf("write: res=%d; error=0x%x\n", res, error);
        }
        return -1;
    }
    return 0;
}

static
int do_read(int p, int start, int size, int* data)
{
    off_t pos;
    int res;

    pos=lseek(p, sizeof(int)*start+devstart, SEEK_SET);
    if (pos==(off_t)-1) {
        perror("do_read::lseek");
        return 1;
    }
    res=read(p, data, size*sizeof(int));
    if (res!=size*sizeof(int)) {
        u_int32_t error;
        ioctl(p, SIS1100_LAST_ERROR, &error);
        if (res<0) {
            printf("read: %s; error=0x%x\n", strerror(errno), error);
        } else {
            printf("read: res=%d; error=0x%x\n", res, error);
        }
        return -1;
    }
    return 0;
}

static
int read_word(int p, int addr)
{
    off_t pos;
    int res, val;

    pos=lseek(p, sizeof(int)*addr+devstart, SEEK_SET);
    if (pos==(off_t)-1) {
        perror("read_word::lseek");
        return 1;
    }
    res=read(p, &val, sizeof(int));
    if (res!=sizeof(int)) {
        u_int32_t error;
        ioctl(p, SIS1100_LAST_ERROR, &error);
        if (res<0) {
            printf("read: %s; error=0x%x\n", strerror(errno), error);
        } else {
            printf("read: res=%d; error=0x%x\n", res, error);
        }
        return -1;
    }
    return val;
}

static void
set_break(int p, int size)
{
    struct vmespace space;
    int res;
    space.am=0xb;
    space.datasize=4;
    space.swap=1;
    space.mapit=0;
    space.mindmalen=size;
    res=ioctl(p, SETVMESPACE, &space);
    if (res<0)
        printf("ioctl(SETVMESPACE): %s\n", strerror(errno));
}

static int
do_check(int p, int num, int mstart, int* obuf, int* ibuf)
{
/*
    printf("%7x %p %7x %p\n", num, obuf, mstart, ibuf);
*/
    if (do_write(p, mstart, num, obuf)) {
        printf("\nwrite failed\n");
        return -1;
    }
    if (do_read(p, mstart, num, ibuf)) {
        printf("\nread failed\n");
        return -1;
    }

    if (bcmp(obuf, ibuf, num*sizeof(int)!=0)) {
        int i, fehler=0, dstart, dend;

        printf("\nmismatch at num=%d\n", num);

        for (i=0; i<num; i++) {
            if (obuf[i]!=ibuf[i]) {fehler=i; break;}
        }
        dstart=fehler-5; if (dstart<0) dstart=0;
        dend=fehler+5; if (dend>num) dend=num;
        for (i=dstart; i<dend; i++)
            printf("[%d] %08x --> %08x --> %08x\n", i,
                    obuf[i], read_word(p, mstart+i), ibuf[i]);
        return -1;
    }
    return 0;
}

static
int generate_max(int max)
{
    int r, bits, _max, mask;
    int m;
    
    m=(random()&7)*3;
    for (_max=max, bits=0; _max && (bits<m); _max>>=1) bits++;
    for (mask=0; bits; bits--) {
        mask<<=1;
        mask|=1;
    }
    do {
        r=random()&mask;
    } while (r>max);
    return r;
}

static
int generate_int(int max)
{
    int r, bits, _max, mask;

    for (_max=max, bits=0; _max; _max>>=1) bits++;
    for (mask=0; bits; bits--) {
        mask<<=1;
        mask|=1;
    }
    do {
        r=random()&mask;
    } while (r>max);
    return r;
}

int main(int argc, char* argv[])
{
    int *ibuf=0, *obuf=0;
    u_int32_t max=0, _max;
    int maxbits, size, *istart, *ostart, mstart, rword;
    int devtype;

    sis1100=-1;
    maxsize=0;

    if (getoptions(argc, argv)<0) goto fehler;

    if (ioctl(sis1100, SIS1100_DEVTYPE, &devtype)<0) {
        printf("ioctl(SIS1100_DEVTYPE): %s\n", strerror(errno));
        goto fehler;
    }
    switch (devtype) {
    case 0: printf("using VME Device\n"); break;
    case 1: printf("using RAM Device\n"); break;
    case 2: printf("cannot use SHARC Device\n"); goto fehler;
    default:
        printf("cannot use unknown device %d\n", devtype);
        goto fehler;
    }

    switch (devtype) {
    case 0:
        max=0x04000000;
        devstart=VMESTART;
        set_break(sis1100, -1);
        break;
    case 1:
/*
        max=lseek(sis1100, 0, SEEK_END);
        if (max==(off_t)-1) {
            perror("lseek(0, SEEK_END)");
            goto fehler;
        }
*/
        {
        if (ioctl(sis1100, SIS1100_MAPSIZE, &max)) {
            perror("ioctl(MAPSIZE)");
            return 1;
        }
        devstart=0;
        }
        break;
    }
    printf("usable size is 0x%08x (%d MByte)\n", max, max/(1<<20));
    if (maxsize) {
        printf("used size is %08x (%d MByte)\n", maxsize, maxsize/(1<<20));
        max=maxsize;
    }
    max/=sizeof(int);
    maxbits=-1;
    for (_max=max; _max; _max>>=1) maxbits++;
    printf("max=0x%08x, maxbits=%d\n", max, maxbits);

    ibuf=calloc(max, sizeof(int));
    obuf=calloc(max, sizeof(int));
    hbuf=calloc(max, sizeof(int));
    if (!ibuf || !obuf || !hbuf) {
        perror("calloc");
        goto fehler;
    }

    if (do_write(sis1100, 0, max, obuf)) {
        printf("initial write failed\n");
        goto fehler;
    }

    srandom(17);

    while (1) {
        int i;
        size=generate_max(max);
        ostart=obuf+generate_int(max-size);
        mstart=generate_int(max-size);
        istart=ibuf+generate_int(max-size);
        rword=random();
        for (i=0; i<size; i++) ostart[i]=rword+i;
        if (do_check(sis1100, size, mstart, ostart, istart)<0) break;
    }

fehler:
    if (obuf) free(obuf);
    if (ibuf) free(ibuf);
    if (hbuf) free(hbuf);
    if (sis1100) close(sis1100);
    return 0;
}

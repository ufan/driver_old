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

#define IVAL 1

#define VMESTART 0x84000000

int sis1100, interval;
off_t devstart;
size_t maxsize;

static void
printusage(int argc, char* argv[])
{
    printf("usage: %s [-i interval][-s maxsize] sis1100_path\n",
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
    const char* args="i:s:";

    optarg=0; errflag=0;
    
    while (!errflag && ((c=getopt(argc, argv, args))!=-1)) {
        switch (c) {
        case 'i': interval=atoi(optarg); break;
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
        return 1;
    }
    return 0;
}

static int do_read(int p, int start, int size, int* data)
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
        return 1;
    }
    return 0;
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
do_check(int p, int num, int* ibuf, int* obuf)
{
    int val0, stopsec, i, loops;
    struct timeval start, stop;

    printf("%d", num); fflush(stdout);

    val0=random();
    for (i=0; i<num; i++) obuf[i]=i+val0;

    if (do_write(p, 0, num, obuf)) {
        printf("\ni_write failed\n");
        return -1;
    }
    if (do_read(p, 0, num, ibuf)) {
        printf("\ni_read failed\n");
        printf("p=%d, num=%d, ibuf=%p\n", p, num, ibuf);
        return -1;
    }

    if (bcmp(obuf, ibuf, num*sizeof(int)!=0)) {
        printf("\nmismatch at num=%d\n", num);
        return -1;
    }

    loops=0;
    gettimeofday(&start, 0);
    stopsec=start.tv_sec+interval;
    do {
        if (do_write(p, 0, num, obuf)) {
            printf("\nwrite failed\n");
            return -1;
        }
        gettimeofday(&stop, 0);
        loops++;
    } while (stop.tv_sec<stopsec);
    printf(" %d", loops); fflush(stdout);

    loops=0;
    gettimeofday(&start, 0);
    stopsec=start.tv_sec+interval;
    do {
        if (do_read(p, 0, num, ibuf)) {
            printf("\nread failed\n");
            return -1;
        }
        gettimeofday(&stop, 0);
        loops++;
    } while (stop.tv_sec<stopsec);
    printf(" %d\n", loops); fflush(stdout);

    return 0;
}

int main(int argc, char* argv[])
{
    int *ibuf=0, *obuf=0;
    u_int32_t max=0;
    int size;
    int devtype;

    sis1100=-1;
    interval=IVAL;
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
    max/=sizeof(int);
    printf("usable size is 0x%08x (%d MWords)\n", max, max/(1<<20));
    if (maxsize) max=maxsize;
    printf("using %d Words\n", max);

    ibuf=obuf=0;
    ibuf=calloc(max, sizeof(int));
    if (!ibuf) {
        printf("calloc %d bytes for ibuf: %s\n", max*sizeof(int), strerror(errno));
        goto fehler;
    }
    obuf=calloc(max, sizeof(int));
    if (!obuf) {
        printf("calloc %d bytes for obuf: %s\n", max*sizeof(int), strerror(errno));
        goto fehler;
    }
    printf("ibuf=%p, obuf=%p\n", ibuf, obuf);

    if (do_write(sis1100, 0, max, obuf)) {
        printf("initial write failed\n");
        goto fehler;
    }

    set_break(sis1100, 1);
    for (;;) {
        for (size=1; size<=max; size++) {
            if (do_check(sis1100, size, ibuf, obuf)) {
                printf("\nfailed\n");
                goto fehler;
            }
        }
    }

fehler:
    if (obuf) free(obuf);
    if (ibuf) free(ibuf);
    if (sis1100) close(sis1100);
    return 0;
}

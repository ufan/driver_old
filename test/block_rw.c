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

#if SIS3100_Version < 200
enum sis1100_subdev {sis1100_subdev_vme, sis1100_subdev_ram,
    sis1100_subdev_dsp};
#endif

off_t devstart;
int count;
int width;
int loops;
int addr;
int am;
int use_dma;
int read_type;
int print;
char* dev_path;
int p;

static void
printusage(int argc, char* argv[])
{
    printf("usage: %s\n"
        " [-c count (number of words with width w); must be>0]\n"
        " [-w width default: 4]\n"
        " [-l loops default: 1]\n"
        " [-a addr default: 0]\n"
        " [-m addr_modifier default: 0xB]\n"
        " [-d 1|0 (use dma); default: 0]\n"
        " [-r 1|2|3 (1=read 2=write 3=both); default: write;read;compare]\n"
        " [-p (print values)]\n"
        " sis1100_path\n",
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
    const char* args="c:w:l:a:m:d:r:p";

    count=0;
    width=4;
    loops=1;
    addr=0;
    am=0xb;
    use_dma=0;
    read_type=3;
    print=0;

    optarg=0; errflag=0;
    
    while (!errflag && ((c=getopt(argc, argv, args))!=-1)) {
        switch (c) {
        case 'c': count=strtoul(optarg, 0, 0); break;
        case 'w': width=atoi(optarg); break;
        case 'l': loops=atoi(optarg); break;
        case 'a': addr=strtoul(optarg, 0, 0); break;
        case 'm': am=strtoul(optarg, 0, 0); break;
        case 'd': use_dma=atoi(optarg); break;
        case 'r': read_type=atoi(optarg); break;
        case 'p': print=1; break;
        default: errflag=1;
        }
    }

    if (errflag || optind!=argc-1 || count<=0) {
        printusage(argc, argv);
        return -1;
    }
    dev_path=argv[optind];

    return 0;
}

static int
do_write(int p, char* buf, int count, int width, int addr, int loop)
{
    int start=random();
    int num=count*width;
    int res, i;

    for (i=0; i<num; i++) {
        buf[i]=start++;
    }
    if (!loop) printf("write %d bytes\n", num);
    if (lseek(p, devstart+addr, SEEK_SET)==((off_t)-1)) {
        printf("lseek to %d: %s\n", addr, strerror(errno));
        return -1;
    }
    res=write(p, buf, num);
    if (res<0) {
        printf("write %d bytes to 0x%08x: %s\n", num, addr, strerror(errno));
        return -1;
    } else if (res!=num) {
        printf("wrote only %d bytes\n", res);
        return -1;
    }
    return 0;
}

static int
do_read(int p, char* buf, int count, int width, int addr, int loop)
{
    int num=count*width;
    int res;

    if (!loop) printf("read %d bytes\n", num);
    if (lseek(p, devstart+addr, SEEK_SET)==((off_t)-1)) {
        printf("lseek to 0x%08x: %s\n", addr, strerror(errno));
        return -1;
    }
    res=read(p, buf, num);
    if (res<0) {
        printf("read %d bytes from 0x%08x: %s\n", num, addr, strerror(errno));
        return -1;
    } else if (res!=num) {
        printf("read only %d bytes\n", res);
        return -1;
    }
    return 0;
}

static int
do_compare(char* ibuf, char* obuf, int count, int width, int loop)
{
    int num=count*width;
    int i;
    
    if (bcmp(ibuf, obuf, num)) {
        for (i=0; i<num; i++) {
            if (ibuf[i]!=obuf[i]) {
                printf("[%d] %x->%x\n", i, obuf[i], ibuf[i]);
            }
        }
        return -1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    int l;
    u_int32_t max=0;
    char *ibuf, *obuf;
    struct vmespace space;
    enum sis1100_subdev devtype;

    if (getoptions(argc, argv)) return 1;

    if ((p=open(dev_path, O_RDWR, 0))<0) {
        printf("open \"%s\": %s\n", dev_path, strerror(errno));
        return 1;
    }

    if (ioctl(p, SIS1100_DEVTYPE, &devtype)<0) {
        printf("ioctl(SIS1100_DEVTYPE): %s\n", strerror(errno));
        return 1;
    }
    switch (devtype) {
    case sis1100_subdev_vme: printf("using VME Device\n"); break;
    case sis1100_subdev_ram: printf("using RAM Device\n"); break;
    case sis1100_subdev_ctrl: printf("cannot use CONTROL Device\n"); return 1;
    case sis1100_subdev_dsp: printf("cannot use DSP Device\n"); return 1;
    default:
        printf("cannot use unknown device %d\n", devtype);
        return 1;
    }
    switch (devtype) {
    case sis1100_subdev_vme:
        max=0x04000000;
        devstart=VMESTART;
        break;
    case sis1100_subdev_ram:
        {
        if (ioctl(p, SIS1100_MAPSIZE, &max)) {
            printf("ioctl(MAPSIZE): %s\n", strerror(errno));
            return 1;
        }
        devstart=0;
        }
        break;
    }
    printf("usable size is 0x%08x (%d MByte)\n", max, max/(1<<20));
    if (count*width>max) {
        printf("count is too large.\n");
        return 1;
    }
    ibuf=obuf=0;
    if (read_type&1) {
        ibuf=calloc(count, width);
        if (!ibuf) {
            printf("cannot allocate memory for read buffer\n");
            return 1;
        }
    }

    if (read_type&2) {
        obuf=calloc(count, width);
        if (!obuf) {
            printf("cannot allocate memory for write buffer\n");
            return 1;
        }
    }

    space.am=am;
    space.datasize=width;
    space.swap=1;
    space.mapit=0;
    space.mindmalen=!!use_dma;
    if (ioctl(p, SETVMESPACE, &space)) {
        printf("ioctl(SETVMESPACE): %s\n", strerror(errno));
        return 1;
    }

    for (l=0; l<loops; l++) {
        /*if (loops>1) printf("loop %d\n", l);*/
        if (read_type&2) {
            if (do_write(p, obuf, count, width, addr, l)) return 2;
        }

        if (read_type&1) {
            if (do_read(p, ibuf, count, width, addr, l)) return 2;
        }

        if ((read_type&3)==3) {
            if (do_compare(ibuf, obuf, count, width, l)) return 2;
        }
    }

    if (print) {
        int i;
        switch (width) {
        case 1:
            for (i=0; i<count; i++) printf("%02x ", ((u_int8_t*)ibuf)[i]);
            break;
        case 2:
            for (i=0; i<count; i++) printf("%04x ", ((u_int16_t*)ibuf)[i]);
            break;
        case 4:
            for (i=0; i<count; i++) printf("%08x ", ((u_int32_t*)ibuf)[i]);
            break;
        }
        printf("\n");
    }

    if (obuf) free(obuf);
    if (ibuf) free(ibuf);
    close(p);
    return 0;
}

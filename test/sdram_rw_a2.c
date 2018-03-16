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
#define PORT 8899

#undef USE_VME

#define VMESTART 0x84000000

int sis1100, sock, interval;
FILE* log;
off_t devstart;
size_t maxsize;

static void
printusage(int argc, char* argv[])
{
    fprintf(stderr, "usage: %s [-l logfile] [-h plothost] [-p plotport]"
        " [-i interval][-s maxsize] sis1100_path\n",
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
    char* logfilename=0;
    char* hostname=0;
    int port=PORT;
    const char* args="l:h:p:i:s:";

    optarg=0; errflag=0;
    
    while (!errflag && ((c=getopt(argc, argv, args))!=-1)) {
        switch (c) {
        case 'l': logfilename=optarg; break;
        case 'h': hostname=optarg; break;
        case 'p': port=atoi(optarg); break;
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
        fprintf(stderr, "open \"%s\": %s\n", sis1100_path, strerror(errno));
        return -1;
    }

    if (logfilename) {
        if (strcmp(logfilename, "-")==0)
            log=stderr;
        else {
            if ((log=fopen(logfilename, "w"))==0) {
                fprintf(stderr, "fopen \"%s\": %s\n",
                        logfilename, strerror(errno));
                return -1;
            }
        }
    }

    if (hostname) {
        struct sockaddr_in addr;
	struct hostent *he;
        in_addr_t iaddr;

        addr.sin_family=AF_INET;
        addr.sin_port=htons(port);
        iaddr=inet_addr(hostname);
        he=0;
        if (iaddr==(in_addr_t)-1) {
	    he=gethostbyname(hostname);
	    if (!he) {
                herror("gethostbyname");
                return -1;
            }
            /*iaddr=*(he->h_addr_list[0]);*/
            iaddr=*(in_addr_t*)(he->h_addr_list[0]);
        }
        addr.sin_addr.s_addr=iaddr;
        sock=socket(addr.sin_family, SOCK_STREAM, 0);
        if (sock<0) {perror("socket"); return -1;}
        if (connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))<0) {
            perror("connect"); return -1;
        }
        fprintf(stderr, "connected\n");
    }

    return 0;
}

static int
xsend(int s, int n, int* v)
{
    int res, rest=n*4;
    char* p=(char*)v;
    while (rest) {
        res=send(s, p, rest, 0);
        if (res<0) {
            if (errno!=EINTR) {
                perror("send");
                return -1;
            } else
                res=0;
        } else if (res==0) {
            fprintf(stderr, "broken pipe\n");
            return -1;
        }
        rest-=res;
        p+=res;
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
            fprintf(stderr, "read: %s; error=0x%x\n", strerror(errno), error);
        } else {
            fprintf(stderr, "read: res=%d; error=0x%x\n", res, error);
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
        fprintf(stderr, "ioctl(SETVMESPACE): %s\n", strerror(errno));
}
#if 1
static float
calc_speed(struct timeval start, struct timeval stop, int num, int loops,
        char* text)
{
    int ints;
    float secs, s, speed;
    char* unit;

    ints=num*loops;
    secs=(stop.tv_sec-start.tv_sec)+(stop.tv_usec-start.tv_usec)/1000000.;
    s=ints/secs*4; unit="";
    speed=s;
    if (speed>1024.) {speed/=1024.; unit="K";}
    if (speed>1024.) {speed/=1024.; unit="M";}
    fprintf(stderr, " %s %.2f %sByte/s", text, speed, unit);
    return s;
}
#else
static float
calc_speed(struct timeval start, struct timeval stop, int num, int loops,
        char* text)
{
    float usecs, s;

    usecs=(stop.tv_sec-start.tv_sec)*1000000.+(stop.tv_usec-start.tv_usec);
    s=usecs/loops;
    fprintf(stderr, " %s %f s", text, s);
    return s;
}
#endif

static int
do_check(int p, int num, int* ibuf, int* obuf, int max, int sock)
{
    int val0, stopsec, loops, i, n;
    struct timeval start, stop;
    const N=8;
    float v[N];

    n=0;
    val0=random();
    for (i=0; i<num; i++) obuf[i]=i+val0;
#if 0
    if (do_write(p, 0, num, obuf)) {
        fprintf(stderr, "\nwrite failed\n");
        return -1;
    }
    if (do_read(p, 0, num, ibuf)) {
        fprintf(stderr, "\nread failed\n");
        return -1;
    }
#endif
#if 0
    if (bcmp(obuf, ibuf, num*sizeof(int)!=0)) {
        fprintf(stderr, "\nmismatch at num=%d\n", num);
        return -1;
    }
#endif

    set_break(p, 0);
    loops=0;
    gettimeofday(&start, 0);
    stopsec=start.tv_sec+interval;
    do {
        if (do_write(p, 0, num, obuf)) {
            fprintf(stderr, "\nwrite failed\n");
            return -1;
        }
        gettimeofday(&stop, 0);
        loops++;
    } while (stop.tv_sec<stopsec);
    v[n]=calc_speed(start, stop, num, loops, "w");
    n++;

    if (n>=N) {
        fprintf(stderr, "do_check: N too small (n=%d)\n", n);
        return -1;
    }
    loops=0;
    gettimeofday(&start, 0);
    stopsec=start.tv_sec+interval;
    do {
        if (do_read(p, 0, num, ibuf)) {
            fprintf(stderr, "\nread failed\n");
            return -1;
        }
        gettimeofday(&stop, 0);
        loops++;
    } while (stop.tv_sec<stopsec);
    v[n]=calc_speed(start, stop, num, loops, "r");
    n++;

    if (n>=N) {
        fprintf(stderr, "do_check: N too small (n=%d)\n", n);
        return -1;
    }

    set_break(p, 1);
    loops=0;
    gettimeofday(&start, 0);
    stopsec=start.tv_sec+interval;
    do {
        if (do_write(p, 0, num, obuf)) {
            fprintf(stderr, "\nwrite with DMA failed\n");
            return -1;
        }
        gettimeofday(&stop, 0);
        loops++;
    } while (stop.tv_sec<stopsec);
    v[n]=calc_speed(start, stop, num, loops, "w");
    n++;

    if (n>=N) {
        fprintf(stderr, "do_check: N too small (n=%d)\n", n);
        return -1;
    }
    loops=0;
    gettimeofday(&start, 0);
    stopsec=start.tv_sec+interval;
    do {
        if (do_read(p, 0, num, ibuf)) {
            fprintf(stderr, "\nread with DMA failed; loop=%d\n", loops);
            return -1;
        }
        gettimeofday(&stop, 0);
        loops++;
    } while (stop.tv_sec<stopsec);
    v[n]=calc_speed(start, stop, num, loops, "r");
    n++;

    if (log) {
        fprintf(log, "%6d", num);
        for (i=0; i<n; i++)
            fprintf(log, " %f\n", v[i]);
        fprintf(log, "\n");
        fflush(log);
    }
    if (sock>=0) {
        if (xsend(sock, 1, &n)<0) return -1;
        if (xsend(sock, 1, &num)<0) return -1;
        for (i=0; i<n; i++) {
            if (xsend(sock, 1, (int*)&v[i])<0) return -1;
        }
    }
    return 0;
}

static int
wenden(int val, int bits)
{
    int _val=0;
    while (bits) {
        _val<<=1;
        if (val&1) _val|=1;
        val>>=1;
        bits--;
    }
    return _val;
}

int main(int argc, char* argv[])
{
    int *ibuf=0, *obuf=0;
    u_int32_t max=0, _max;
    int maxbits, bitwidth, bitshift, cc, ccw, size;
    int devtype;

    sis1100=-1;
    sock=-1;
    interval=IVAL;
    log=0;
    maxsize=0;

    if (getoptions(argc, argv)<0) goto fehler;

    if (ioctl(sis1100, SIS1100_DEVTYPE, &devtype)<0) {
        fprintf(stderr, "ioctl(SIS1100_DEVTYPE): %s\n", strerror(errno));
        goto fehler;
    }
    switch (devtype) {
    case 0: fprintf(stderr, "using VME Device\n"); break;
    case 1: fprintf(stderr, "using RAM Device\n"); break;
    case 2: fprintf(stderr, "cannot use SHARC Device\n"); goto fehler;
    default:
        fprintf(stderr, "cannot use unknown device %d\n", devtype);
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
    fprintf(stderr, "usable size is %08x (%d MByte)\n", max, max/(1<<20));
    if (maxsize) {
        fprintf(stderr, "used size is %08x (%d MByte)\n", maxsize, maxsize/(1<<20));
        max=maxsize;
    }

    max/=sizeof(int);
    maxbits=-1;
    for (_max=max; _max; _max>>=1) maxbits++;
    fprintf(stderr, "max=0x%08x, maxbits=%d\n", max, maxbits);

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

    if (do_write(sis1100, 0, max, obuf)) {
        fprintf(stderr, "initial write failed\n");
        goto fehler;
    }

    for (bitwidth=1; bitwidth<=maxbits; bitwidth++) {
        for (bitshift=0; bitshift<=(maxbits-bitwidth); bitshift++) {
            for (cc=1; cc<(1<<bitwidth); cc++) {
                if (!(cc&1) || !(cc&(1<<(bitwidth-1)))) continue;
                ccw=wenden(cc, bitwidth);
                size=ccw<<bitshift;
                /*if (size&1) continue;*/
                fprintf(stderr, "%6d", size);
                if (do_check(sis1100, size, ibuf, obuf, max, sock)) {
                    fprintf(stderr, "\nfailed\n");
                    goto fehler;
                }
                fprintf(stderr, "\n");
            }
        }
    }

fehler:
    if (obuf) free(obuf);
    if (ibuf) free(ibuf);
    if (sis1100) close(sis1100);
    if (log) fclose(log);
    if (sock) close(sock);
    return 0;
}

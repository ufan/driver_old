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

char* pathname;


/****************************************************************************/
static int getopts(int argc, char* argv[])
{
        extern char *optarg;
        extern int optind;
        extern int opterr;
        extern int optopt;

        const char* optstring="f:";

        int c, errflag = 0;

        pathname=0;
        while (!errflag && ((c = getopt(argc, argv, optstring)) != -1)) {
                switch (c) {
                case 'f': pathname=optarg;
                        break;
                case '?':
                case 'h':
                        errflag=1;
                }
        }
        if (errflag || !pathname) {
                fprintf(stderr, "usage: %s -f pathname\n", argv[0]);
                return -1;
        } else {
                return 0;
        }
}
/****************************************************************************/
static off_t select_size(off_t max)
{
        off_t size;
        size=random()&0x1fffff;
        return size;
}
/****************************************************************************/
static off_t select_start(off_t max, off_t size)
{
        off_t diff=max-size;
        off_t mask, start;
        mask=1;
        while (mask<diff) {mask<<=1; mask++;}
        do {
                start=random()&mask;
        } while (start>diff);
        return start;
}
/****************************************************************************/
static void fill_random(u_int32_t* buf, off_t size)
{
        /*off_t i;*/
        for (; size; buf++, size--) *buf=random();
        /*for (i=0; size; buf++, size--, i++) *buf=i;*/
}
/****************************************************************************/
static int check(u_int32_t* obuf, u_int32_t* ibuf, off_t size)
{
        off_t i;
        int count;

        if (!bcmp(obuf, ibuf, size*sizeof(int))) {
                return 0;
        }
        for (i=0, count=0; (i<size) && (count<10); i++) {
                if (obuf[i]!=ibuf[i]) {
                        fprintf(stderr, "check: [0x%08Lx] 0x%08x --> 0x%08x\n",
                                i, obuf[i], ibuf[i]);
                        count++;
                }
        }
        return -1;
}
/****************************************************************************/
int main(int argc, char* argv[])
{
        u_int32_t *ibuf, *obuf;
        int p, res;
        off_t max;

        srandom(17);
        if (getopts(argc, argv)<0) return 1;
        if ((p=open(pathname, O_RDWR, 0))<0) {
                fprintf(stderr, "open %s: %s\n", pathname, strerror(errno));
                return 1;
        }
        max=lseek(p, 0, SEEK_END);
        if (max==(off_t)-1) {
                perror("lseek(0, SEEK_END)");
                return 1;
        }
        printf("size of sdram is %08Lx (%Ld MByte)\n", max, max/(1<<20));
        max/=sizeof(int);
        ibuf=malloc(max*sizeof(int));
        printf("ibuf=0x%08X\n", (unsigned int)ibuf);
        obuf=malloc(max*sizeof(int));
        printf("obuf=0x%08X\n", (unsigned int)obuf);
        if (!ibuf || !obuf) {
                perror("calloc");
                return 1;
        }
        while (1) {
                off_t start_w, start_r, start_m, size;

                size=select_size(max);
                start_w=select_start(max, size);
                start_r=select_start(max, size);
                start_m=select_start(max, size);
                fill_random(obuf+start_w, size);
                if (lseek(p, sizeof(int)*start_m, SEEK_SET)==(off_t)-1) {
                        fprintf(stderr, "lseek(0x%08Lx): %s\n",
                                sizeof(int)*start_m, strerror(errno));
                        return 1;
                }
                res=write(p, obuf+start_w, size*sizeof(int));
                if (res!=size*sizeof(int)) {
                        fprintf(stderr, "write: %s\n", strerror(errno));
                        return 1;
                }
                if (lseek(p, -size*sizeof(int), SEEK_CUR)==(off_t)-1) {
                        fprintf(stderr, "lseek(0x%08Lx (rel)): %s\n",
                                -size*sizeof(int), strerror(errno));
                        return 1;
                }
                res=read(p, ibuf+start_r, size*sizeof(int));
                if (res!=size*sizeof(int)) {
                        fprintf(stderr, "read: %s\n", strerror(errno));
                        return 1;
                }
                if (check(obuf+start_w, ibuf+start_r, size))
                        return 1;
                printf("."); fflush(stdout);
        }
        return 0;
}

#ifdef __linux__
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#define LINUX_LARGEFILE O_LARGEFILE
#else
#define LINUX_LARGEFILE 0
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "dev/pci/sis1100_var.h"

struct caen_type {
    int typ;
    char* name;
    char* descr;
};

struct caen_type caen_types[]={
    {0x34, "V550", "C-RAMS"},            /* 64k */
    {0x3c, "V551B", "C-RAMS Sequencer"},
    {0x48, "V729A", "40 MHz ADC"},
    {0x12e, "V693", "Multihit TDC"},
    {0, 0, 0} 
};

/****************************************************************************/
static int find_caen_name(int typ)
{
    int i;
    for (i=0; caen_types[i].name && caen_types[i].typ!=typ; i++);
    if (caen_types[i].typ==typ)
        return i;
    else
        return -1;
}
/****************************************************************************/
static int check_sis(int p, u_int32_t addr)
{
    u_int32_t _addr;
    u_int32_t v1;
    int res, version=0;
    char* name=0;
    struct vmespace space;

    space.am=9;
    space.datasize=4;
    space.swap=1;
    space.mapit=0;
    space.mindmalen=-1;

    if (ioctl(p, SIS1100_SETVMESPACE, &space)<0) {
        perror("SETVMESPACE");
        return -1;
    }
    _addr=addr+0x4;
    if (ioctl(p, VME_PROBE, &_addr)<0) {
        /*perror("VME_PROBE");*/
        return 0;
    }

    if (lseek(p, addr+0x4, SEEK_SET)==(off_t)-1) {
        perror("lseek");
        return -1;
    }

    res=read(p, &v1, 4);
    if (res!=4) {
        fprintf(stderr, "read 0x%x+0x4: %s\n", addr, strerror(errno));
        return -1;
    }
    switch ((v1>>16)&0xffff) {
    case 0x3300: name="3300"; version=v1&0xffff; break;
    case 0x3600: name="3600"; version=(v1>>12)&0xf; break;
    case 0x3800: name="3800"; version=(v1>>12)&0xf; break;
    case 0x3801: name="3801"; version=(v1>>12)&0xf; break;
    }

    if (name) {
        printf("0x%08x: SIS%s; version %d\n", addr, name, version);
    }
    return name!=0;
}
/****************************************************************************/
int main(int argc, char* argv[])
{
    u_int32_t addr;
    int p, num, idx, n, res;

    if (argc<2) {
        fprintf(stderr, "usage: %s path [num]\n", argv[0]);
        return 1;
    }
    num=argc>2?atoi(argv[2]):65536;

    if ((p=open(argv[1], O_RDWR|LINUX_LARGEFILE, 0))<0) {
        fprintf(stderr, "open %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    for (addr=0, idx=0, n=0; idx<num; idx++, addr+=0x10000) {
        res=check_sis(p, addr);
        if (res<0) return 0;
        if (res>0) n++;
    }

    printf("%d module%s found\n", n, n==1?"":"s");
    close(p);
    return 0;
}

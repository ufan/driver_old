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
    {0x003, "V259", "multihit pattern/NIM"},
    {0x004, "V259E", "multihit pattern/ECL"},
    {0x001, "V262", "IO reg"},
    {0x01a, "V512", "Logic"},
    {0x018, "V560", "counter"},
    {0x034, "V550", "C-RAMS"},            /* 64k */
    {0x03c, "V551B", "C-RAMS Sequencer"},
    {0x048, "V729A", "40 MHz ADC"},
    {0x12e, "V693", "Multihit TDC"},
    {0x2ff, "V767", "tdc"},
    {0x307, "V775", "tdc"},
    {0x311, "V785", "Peak Sensing ADC"},
    {0x318, "V792", "qdc"},
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
static int check_caen_new(int p, u_int32_t addr, u_int32_t offs)
{
    u_int32_t _addr;
    u_int16_t oui_msb, oui, oui_lsb;
    u_int16_t ver;
    u_int16_t id_msb, id, id_lsb;
    u_int16_t rev;
    u_int16_t ser_msb, ser_lsb;
    u_int32_t manu, board_id, serial;
    int res, idx;
    struct vmespace space;

    space.am=0x39;
    space.datasize=2;
    space.swap=1;
    space.mapit=0;
    space.mindmalen=-1;

    _addr=addr+offs;

    if (ioctl(p, SIS1100_SETVMESPACE, &space)<0) {
        perror("SETVMESPACE");
        return -1;
    }

    if (ioctl(p, VME_PROBE, &_addr+0x26)<0) return 0;

    res=pread(p, &oui_msb, 2, _addr+0x26);
    if (res!=2) return 0;

    res=pread(p, &oui, 2, _addr+0x2a);
    if (res!=2) return 0;

    res=pread(p, &oui_lsb, 2, _addr+0x2e);
    if (res!=2) return 0;

    manu=((oui_msb&0xff)<<12)|((oui&0xff)<<8)|(oui_lsb&0xff);
    if (manu!=0x40e6) return 0;

    res=pread(p, &ver, 2, _addr+0x32);
    if (res!=2) return 0;

    res=pread(p, &id_msb, 2, _addr+0x36);
    if (res!=2) return 0;

    res=pread(p, &id, 2, _addr+0x3a);
    if (res!=2) return 0;

    res=pread(p, &id_lsb, 2, _addr+0x3e);
    if (res!=2) return 0;

    board_id=((id_msb&0xff)<<12)|((id&0xff)<<8)|(id_lsb&0xff);
    res=pread(p, &rev, 2, _addr+0x4e);
    if (res!=2) return 0;

    res=pread(p, &ser_msb, 2, _addr+0xf02);
    if (res!=2) return 0;

    res=pread(p, &ser_lsb, 2, _addr+0xf06);
    if (res!=2) return 0;

    serial=((ser_msb&0xff)<<8)|(ser_lsb&0xff);
    idx=find_caen_name(board_id);
    if (idx>=0)
        printf("0x%08x: %03x CAEN %-6s; version=%2d; serial=%3d; revision=%d (%s)\n",
            addr, board_id, caen_types[idx].name, ver, serial, rev, caen_types[idx].descr);
    else
        printf("0x%08x: %03x CAEN unknown type 0x%x; version=%2d; serial=%3d; revision=%d\n",
            addr, board_id, board_id, ver, serial, rev);
    return 1;
}
/****************************************************************************/
static int check_caen_old(int p, u_int32_t addr)
{
    u_int16_t v[3];
    int res;
    struct vmespace space;

    space.am=0x39;
    space.datasize=2;
    space.swap=1;
    space.mapit=0;
    space.mindmalen=-1;

    if (ioctl(p, SIS1100_SETVMESPACE, &space)<0) {
        perror("SETVMESPACE");
        return -1;
    }

    res=pread(p, v, 6, addr+0xfa);
    if (res!=6) {
        /*fprintf(stderr, "read 0x%x+0xfa: %s\n", addr, strerror(errno));*/
        return 0;
    }

    if (v[0]==0xfaf5) {
        int typ, manf, ser, ver, idx;

        typ=v[1]&0x3ff;
        manf=(v[1]>>10)&0x3f;
        ser=v[2]&0xfff;
        ver=(v[2])>12&0xf;
        idx=find_caen_name(typ);
        if (idx>=0)
            printf("0x%08x: %03x CAEN %-6s; version=%2d; serial=%3d (%s)\n",
                addr, typ, caen_types[idx].name, ver, ser, caen_types[idx].descr);
        else
            printf("0x%08x: %03x CAEN; unknown type 0x%x; version=%2d; serial=%3d\n",
                addr, typ, typ, ver, ser);
        return 1;
    }
    return 0;
}
/****************************************************************************/
static int check_caen(int p, u_int32_t addr)
{
    volatile int res;
    res=check_caen_old(p, addr);
    /*printf("check_caen_old(0x%08x): %d\n", addr, res);*/
    if (res) return res;
    res=check_caen_new(p, addr, 0x1000);
    /*printf("check_caen_new1(0x%08x): %d\n", addr, res);*/
    if (res) return res;
    res=check_caen_new(p, addr, 0x8000);
    /*printf("check_caen_new2(0x%08x): %d\n", addr, res);*/
    if (res) return res;
    return res;
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

    if ((p=open(argv[1], O_RDWR, 0))<0) {
        fprintf(stderr, "open %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    for (addr=0, idx=0, n=0; idx<num; idx++, addr+=0x10000) {
        res=check_caen(p, addr);
        if (res<0) return 0;
        if (res>0) n++;
    }

    printf("%d module%s found\n", n, n==1?"":"s");
    close(p);
    return 0;
}

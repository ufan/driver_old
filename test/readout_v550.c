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
    {0x311, "V785", "Peak Sensing ADC"},
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
static void
fill_pipeent(struct sis1100_pipelist* ent, int am, int size, u_int32_t addr)
{
    ent->head=((0x00f00000<<size)&0x0f000000)<<(addr&3)|0x00010000;
    ent->am=am;
    ent->addr=addr;
}
/****************************************************************************/
static int check_caen_rom(int p, u_int32_t addr)
{
    struct sis1100_pipelist list[10];
    struct sis1100_pipe pipe;
    u_int16_t ver;
    u_int16_t rev;
    u_int32_t manu, board_id, serial;
    int idx, i;
    u_int32_t data[10];
    static const u_int32_t offs[10]={
        0x8026, /*oui_msb*/
        0x802a, /*oui*/
        0x802e, /*oui_lsb*/
        0x8032, /*ver*/
        0x8036, /*id_msb*/
        0x803a, /*id*/
        0x803e, /*id_lsb*/
        0x804e, /*rev*/
        0x8f02, /*ser_msb*/
        0x8f06  /*ser_lsb*/};


    for (i=0; i<10; i++) fill_pipeent(list+i, 9, 2, addr+offs[i]);
    pipe.num=10;
    pipe.list=list;
    pipe.data=data;

    if (ioctl(p, SIS1100_PIPE, &pipe)<0) {
	printf("ioctl(SIS1100_PIPE): %s\n", strerror(errno));
        return -1;
    }
    if (pipe.error) return 0;

    manu=((data[0]&0xff)<<12)|((data[1]&0xff)<<8)|(data[2]&0xff);
    if (manu!=0x40e6) return 0;

    board_id=((data[4]&0xff)<<12)|((data[5]&0xff)<<8)|(data[6]&0xff);
    serial=((data[8]&0xff)<<8)|(data[9]&0xff);
    ver=data[3];
    rev=data[7];
    idx=find_caen_name(board_id);
    if (idx>=0)
        printf("0x%08x: CAEN %-6s; version=%d; serial=%d; revision=%d (%s)\n",
            addr, caen_types[idx].name, ver, serial, rev, caen_types[idx].descr);
    else
        printf("0x%08x: CAEN unknown type 0x%x; version=%d; serial=%d; revision=%d\n",
            addr, board_id, ver, serial, rev);
    return board_id;
}
/****************************************************************************/
static int check_caen(int p, u_int32_t addr)
{
    u_int16_t v[3];
    int res;
    struct vmespace space;

    if ((res=check_caen_rom(p, addr))>0) return res;

    space.am=9;
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
            printf("0x%08x: CAEN %-6s; version=%d; serial=%d (%s)\n",
                addr, caen_types[idx].name, ver, ser, caen_types[idx].descr);
        else
            printf("0x%08x: CAEN; unknown type 0x%x; version=%d; serial=%d\n",
                addr, typ, ver, ser);
        return typ;
    }
    return 0;
}
/****************************************************************************/
static u_int32_t find_caen(int p, int code)
{
    u_int32_t addr;
    int idx, res;

    for (addr=0, idx=0; idx<65536; idx++, addr+=0x10000) {
        res=check_caen(p, addr);
        if (res<0) return 0xffffffffU;
        if (res==code) return addr;
    }
    return 0xffffffffU;
}
/****************************************************************************/
static int
write_16(int p, u_int32_t addr, u_int16_t val)
{
    struct sis1100_vme_req req;
    req.size=2;
    req.am=9;
    req.addr=addr;
    req.data=val;
    req.error=0;
    if (ioctl(p, SIS3100_VME_WRITE, &req)<0) {
        fprintf(stderr, "VME_WRITE(0x%08x, 0x%x)\n", addr, val);
        return -1;
    }
    if (req.error){
        fprintf(stderr, "VME_WRITE(0x%08x, 0x%x): error=0x%x\n",
            addr, val, req.error);
        return -1;
    }
    return 0;
}
/****************************************************************************/
static u_int16_t
read_16(int p, u_int32_t addr)
{
    struct sis1100_vme_req req;
    req.size=2;
    req.am=9;
    req.addr=addr;
    req.data=0;
    req.error=0;
    if (ioctl(p, SIS3100_VME_READ, &req)<0) {
        fprintf(stderr, "VME_READ(0x%08x)\n", addr);
        return -1;
    }
    if (req.error){
        fprintf(stderr, "VME_READ(0x%08x): error=0x%x\n",
            addr, req.error);
        return -1;
    }
    return req.data&0xffff;
}
/****************************************************************************/
static u_int32_t
read_32(int p, u_int32_t addr)
{
    struct sis1100_vme_req req;
    req.size=2;
    req.am=9;
    req.addr=addr;
    req.data=0;
    req.error=0;
    if (ioctl(p, SIS3100_VME_READ, &req)<0) {
        fprintf(stderr, "VME_READ(0x%08x): %s\n", addr, strerror(errno));
        return -1;
    }
    if (req.error){
        fprintf(stderr, "VME_READ(0x%08x): error=0x%x\n",
            addr, req.error);
        return -1;
    }
    return req.data;
}
/****************************************************************************/
static int
setup_v551(int p, u_int32_t addr)
{
    write_16(p, addr+0xc, 2016); /* number of channels */
    return 0;
}
/****************************************************************************/
static int
setup_v550(int p, u_int32_t addr)
{
    write_16(p, addr+0x4, (0x3f<<6)|(0x3f)); /* number of channels */
    write_16(p, addr+0x2, 2);
    return 0;
}
/****************************************************************************/
static int
trigger_v551(int p, u_int32_t addr)
{
    struct sis1100_vme_req req;
    req.size=2;
    req.am=9;
    req.addr=addr+6;
    req.data=0;
    req.error=0;
    if (ioctl(p, SIS3100_VME_WRITE, &req)<0) {
        perror("VME_WRITE(6, 0)");
        return -1;
    }
    if (req.error){
        fprintf(stderr, "VME_WRITE(6, 0): error=0x%x\n", req.error);
        return -1;
    }
    return 0;
}
/****************************************************************************/
static int
read_v550(int p, u_int32_t addr)
{
    int count0, count1, i, val;

    count0=read_16(p, addr+0x10)+10;
    count1=read_16(p, addr+0x12);
    printf("count0=%d; count1=%d\n", count0, count1);
    for (i=0; i<count0; i++) {
        val=read_32(p, addr+0x8);
    }
    for (i=0; i<count1; i++) {
        val=read_32(p, addr+0xc);
    }
    return 0;
}
/****************************************************************************/
static int
stat_v550(int p, u_int32_t addr)
{
    u_int16_t stat;
    stat=read_16(p, addr+0x2);
    printf("status 550=0x%04x\n", stat);
    return 0;
}
/****************************************************************************/
static int
clear_v550(int p, u_int32_t addr)
{
    write_16(p, addr+0x6, 0);
    return 0;
}
/****************************************************************************/
static int
stat_v551(int p, u_int32_t addr)
{
    u_int16_t stat;
    stat=read_16(p, addr+0x8);
    printf("status 551=0x%04x\n", stat);
    return 0;
}
/****************************************************************************/
int main(int argc, char* argv[])
{
    u_int32_t addr_550, addr_551;
    int p, num;

    if (argc<2) {
        fprintf(stderr, "usage: %s path [num]\n", argv[0]);
        return 1;
    }
    num=argc>2?atoi(argv[2]):65536;

    if ((p=open(argv[1], O_RDWR, 0))<0) {
        fprintf(stderr, "open %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    addr_550=find_caen(p, 0x34);
    if (addr_550==0xffffffff) {
        printf("no V550 found\n");
        return 1;
    } else {
        printf("fount V550 at 0x%08x\n", addr_550);
    }
    addr_551=find_caen(p, 0x3c);
    if (addr_551==0xffffffffU) {
        printf("no V551 found\n");
        return 1;
    } else {
        printf("found V551 at 0x%08x\n", addr_551);
    }
    if (setup_v550(p, addr_550)<0) return 1;
    if (setup_v551(p, addr_551)<0) return 1;

    stat_v550(p, addr_550);
    stat_v551(p, addr_551);

    clear_v550(p, addr_550);
    stat_v550(p, addr_550);
    stat_v551(p, addr_551);

    trigger_v551(p, addr_551);
    stat_v550(p, addr_550);
    stat_v551(p, addr_551);

    read_v550(p, addr_550);
    stat_v550(p, addr_550);
    stat_v551(p, addr_551);

    close(p);
    return 0;
}
/****************************************************************************/
/****************************************************************************/

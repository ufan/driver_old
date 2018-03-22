/* $ZEL: maptest.c,v 1.3 2010/08/02 19:25:39 wuestner Exp $ */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "dev/pci/sis1100_var.h"

struct mapinfo {
        u_int32_t header;
        u_int32_t bordaddr;
        int bordsize;
        int modifier;
        int mapidx, mapnum;
        char* mapbase;
        off_t mapsize;
        char* bordbase;
};

/****************************************************************************/
static void
clear_maps(int p)
{
        int i;
        struct sis1100_ctrl_reg reg;

        for (i=0; i<64; i++) {
                /* clear the .adl register; --> mark as unused */
                reg.offset=0x408+16*i;
                reg.val=0;
                ioctl(p, SIS1100_CONTROL_WRITE, &reg);
        }
}
/****************************************************************************/
static void
dump_map(int p, struct mapinfo* map)
{
        int i;
        printf("---------------------------\n");
        printf("header  =0x%08x\n", map->header);
        printf("bordaddr=0x%08x\n", map->bordaddr);
        printf("bordsize=0x%08x\n", map->bordsize);
        printf("modifier=      0x%02x\n", map->modifier);
        printf("mapidx  =  %8d\n", map->mapidx);
        printf("mapnum  =  %8d\n", map->mapnum);
        printf("mapbase =%p\n", map->mapbase);
        printf("mapsize =0x%08lx\n", map->mapsize);
        printf("bordbase=%p\n", map->bordbase);
        printf("\n");

        for (i=map->mapidx; i<map->mapidx+map->mapnum; i++) {
                struct sis1100_ctrl_reg reg;
                u_int32_t offs=0x400+16*i;
                
                printf("idx=%d\n", i);
                reg.offset=offs+0;
                ioctl(p, SIS1100_CONTROL_READ, &reg);
                printf(".hdr=0x%08x\n", reg.val);
                reg.offset=offs+4;
                ioctl(p, SIS1100_CONTROL_READ, &reg);
                printf(".am =0x%08x\n", reg.val);
                reg.offset=offs+8;
                ioctl(p, SIS1100_CONTROL_READ, &reg);
                printf(".adl=0x%08x\n", reg.val);
                reg.offset=offs+12;
                ioctl(p, SIS1100_CONTROL_READ, &reg);
                printf(".adh=0x%08x\n", reg.val);
        }

}
/****************************************************************************/
static int
map_it(int p, struct mapinfo* map)
{
        u_int32_t spacebase;
        u_int32_t spacesize;
        u_int32_t bordoffs;
        struct sis1100_ctrl_reg reg;
        int i;

        /* get size of VME-space */
        if (ioctl(p, SIS1100_MAPSIZE, &spacesize)<0) {
                printf("SIS1100_MAPSIZE: %s\n", strerror(errno));
                return -1;
        }

        /*
        printf("size of VME space=0x%x\n", spacesize);
        */

        spacebase=map->bordaddr & MAPMASK;
        bordoffs=map->bordaddr & OFFMASK;
        map->mapnum=(map->bordsize+bordoffs+MAPSIZE-1)/MAPSIZE;
        map->mapsize=map->mapnum*MAPSIZE;

        /* this code is only to find an unused map entry */
        /* not really necessary */
        for (i=0; i<64; i++) {
                reg.offset=0x408+16*i;
                if (ioctl(p, SIS1100_CONTROL_READ, &reg)<0) {
                        printf("SIS1100_CONTROL_READ: %s\n", strerror(errno));
                        return -1;
                }
                if (reg.error) {
                        printf("SIS1100_CONTROL_READ: error=0x%x\n", reg.error);
                        return -1;
                }
                if (reg.val==0) break;
        }
        if (i>=(64-map->mapnum)) {
                printf("map_it: no maps available\n");
                return -1;
        }
        /*printf("found free entry at %d\n", i);*/



        map->mapidx=i;
        for (i=0; i<map->mapnum; i++) {
                u_int32_t offs=0x400+16*(map->mapidx+i);

                reg.offset=offs+0;
                reg.val=map->header;
                ioctl(p, SIS1100_CONTROL_WRITE, &reg);

                reg.offset=offs+4;
                reg.val=map->modifier;
                ioctl(p, SIS1100_CONTROL_WRITE, &reg);
                reg.offset=offs+8;

                /* the '|0xa5a5' is only used to mark the entry as 'in use' */
                /* the lowest 22 bits are ignored, so we can misuse them */
                reg.val=spacebase+MAPSIZE*i|0xa5a5;
                ioctl(p, SIS1100_CONTROL_WRITE, &reg);

                reg.offset=offs+12;
                reg.val=0;
                ioctl(p, SIS1100_CONTROL_WRITE, &reg);
        }

        map->mapbase=mmap(0,
                        map->mapsize,
                        PROT_READ|PROT_WRITE, MAP_SHARED,
                        p,
                        mapinfo.offset+map->mapidx*MAPSIZE);
        /*                             ^^^^^^^^^^^^^^^^^^^   */
        /*                      only this term is really missing in your code*/


        if (map->mapbase==MAP_FAILED) {
                printf("mmap: %s\n", strerror(errno));
                for (i=0; i<map->mapnum; i++) {
                        reg.offset=0x400+16*(map->mapidx+i)+8;
                        reg.val=0;
                        ioctl(p, SIS1100_CONTROL_WRITE, &reg);
                }
                return -1;
        }
        map->bordbase=map->mapbase+bordoffs;
        return 0;
}
/****************************************************************************/
static void
unmap_it(int p, struct mapinfo* map)
{
        struct sis1100_ctrl_reg reg;
        int i;

        munmap(map->mapbase, map->mapsize);
        for (i=0; i<map->mapnum; i++) {
                reg.offset=0x400+16*(map->mapidx+i)+8;
                reg.val=0;
                ioctl(p, SIS1100_CONTROL_WRITE, &reg);
        }
}
/****************************************************************************/
int main(int argc, char* argv[])
{
        int p, p, i;
        struct mapinfo map[5];
        volatile u_int32_t val;

        if (argc!=3) {
                fprintf(stderr, "usage: %s path controlpath\n", argv[0]);
                return 1;
        }

        if ((p=open(argv[1], O_RDWR, 0))<0) {
            printf("open %s: %s\n", argv[1], strerror(errno));
            return 1;
        }

        /* mark all maps as unused */
        clear_maps(cp);

        map[0].header=0xff010800;
        map[0].bordaddr=0x00222200;
        map[0].bordsize=0x400000;
        map[0].modifier=0x39;

        map[1].header=0xff010800;
        map[1].bordaddr=0x00d00000;
        map[1].bordsize=0x300000;
        map[1].modifier=0x39;

        map[2].header=0xff010800;
        map[2].bordaddr=0xee000000;
        map[2].bordsize=0x400000;
        map[2].modifier=0x9;

        map[3].header=0xff010800;
        map[3].bordaddr=0x00f00000;
        map[3].bordsize=0x100000;
        map[3].modifier=0x39;

        map[4].header=0xff010800;
        map[4].bordaddr=0x00380000;
        map[4].bordsize=0x00200000;
        map[4].modifier=0x39;

        for (i=0; i<5; i++) {
                if (map_it(p, map+i)<0) return 1;
                dump_map(p, map+i);
        }

        /* the real access */
        val=*(u_int16_t*)(map[0].bordbase+0xfa);
        val=*(u_int16_t*)(map[1].bordbase+0xfe02);
        val=*(u_int16_t*)(map[2].bordbase+0x1000);
        val=*(u_int16_t*)(map[3].bordbase+0x0);
        val=*(u_int16_t*)(map[4].bordbase+0x100000);

        for (i=0; i<5; i++) {
                unmap_it(p, map+i);
        }

        close(p);
        return 0;    
}
/****************************************************************************/
/****************************************************************************/

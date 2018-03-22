#define _GNU_SOURCE
#ifdef __linux__
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#define LINUX_LARGEFILE O_LARGEFILE
#else
#define LINUX_LARGEFILE 0
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "dspcode.h"
#include "load_dsp.h"

#define SHARCRAM  0x81200000ULL
#define D48REG    0x81300000ULL

/*===========================================================================*/
static int
load_dsp(int p, u_int8_t* code, off_t addr, size_t nr_bytes)
{
    ssize_t res;
    size_t idx;

    addr+=SHARCRAM;

    for (idx=0; idx<nr_bytes; idx+=6) {
        u_int32_t w0, w12;
        u_int8_t* w=code+idx;
#if 0
        printf("%08x %02x%02x%02x%02x%02x%02x,\n",
                    (addr-SHARCRAM)>>2,
                    w[0], w[1], w[2], w[3], w[4], w[5]);
#endif
        w0=w[5]|(w[4]<<8);
        w12=(w[3] | (w[2]<<8) | (w[1]<<16) | (w[0]<<24) );
        if ((res=pwrite(p, &w0, 4, D48REG))!=4) {
            printf("load_dsp: %s\n", strerror(errno));
            return errno;
        }
        if ((res=pwrite(p, &w12, 4, addr))!=4) {
            printf("load_dsp: %s\n", strerror(errno));
            return errno;
        }
        addr+=4;
    }
    return 0;
}
/*===========================================================================*/
static int
load_dsp_coff(int p, struct code_info* info)
{
    int res, i;

    for (i=0; i<info->nsect; i++) {
        struct section* sect=info->sections+i;
        if (sect->is_code) {
            char name[9];
            u_int32_t addr;
            strncpy(name, sect->s_name, 8);
            name[8]=0;
            printf("loading %s (addr=0x%x size=0x%x)\n",
                    name, sect->s_paddr, sect->s_size);
            addr=sect->s_paddr;
            if (addr==0x20000) {
                addr=0x400100;
                printf("relocated to 0x%x\n", addr);
            }
            if (addr<0x400000) {
                printf("invalid address 0x%x\n", addr);
                return EINVAL;
            }
            res=load_dsp(p, sect->addr, (addr-0x400000)<<2, sect->s_size);
            if (res)
                return res;
        }
    }
    return 0;
}
/*===========================================================================*/
int
load_dsp_file(int p_dsp, const char* codepath)
{
    struct code_info code_info;
    ssize_t res;
    size_t size;
    u_int8_t* code;
    int p;
    u_int16_t magic;

    p=open(codepath, O_RDONLY, 0);
    if (p<0) {
        printf("load_dsp_file \"%s\": %s\n", codepath, strerror(errno));
        return errno;
    }
    res=read(p, &magic, 2);
    if (res!=2) {
        printf("load_dsp_file; read 2 bytes of \"%s\": res=%lld errno=%s\n",
            codepath, (unsigned long long)res, strerror(errno));
        close(p);
        return errno;
    }
    lseek(p, 0, SEEK_SET);

/* works only for little endian! */
    printf("load_dsp_file \"%s\": magic=0x%04x\n", codepath, magic);

    switch (magic) {
    case 0x521c: /* coff */
        printf("using coff\n");
        size=read_dsp_coff(p, codepath, &code, &code_info);
        if ((res=load_dsp_coff(p_dsp, &code_info))) {
            printf("error loading DSP code (from coff)\n");
            return res;
        }
        break;
    case 0x7830: /* ldr (0x...) */
        printf("using ldr\n");
        size=read_dsp_ldr(p, codepath, &code, &code_info);
        if ((res=load_dsp(p_dsp, code, 0, size))) {
            printf("error loading DSP code\n");
            return res;
        }
        break;
    default: /* raw binary ??? */
        printf("using raw\n");
        size=read_dsp_raw(p, codepath, &code, &code_info);
        if ((res=load_dsp(p_dsp, code, 0, size))) {
            printf("error loading DSP code\n");
            return res;
        }
        break;
    };
    if (code_info.free)
        code_info.free(&code_info);

    return 0;
}
/*===========================================================================*/
int main(int argc, char* argv[])
{
    const char* name;
    int p_dsp;

    if (argc!=3) {
        printf("usage: %s path_to_sis_dsp filename\n", argv[0]);
        return 1;
    }
/*
    if ((p_dsp=open(argv[1], O_RDWR|LINUX_LARGEFILE, 0))<0) {
        printf("open \"%s\": %s\n", argv[1], strerror(errno));
        printf("here\n");
        return 2;
    }
*/
    if ((p_dsp=open(argv[1], O_RDWR, 0))<0) {
        printf("open \"%s\": %s\n", argv[1], strerror(errno));
        printf("here\n");
        return 2;
    }
    load_dsp_file(p_dsp, argv[2]);
}
/*===========================================================================*/
/*===========================================================================*/

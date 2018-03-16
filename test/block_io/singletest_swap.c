/* $ZEL$ */

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <limits.h>
#include <libgen.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>

#include <sis1100_var.h>

#define SWAP_BYTES(x) \
    ((((x)>>24)&0x000000ff)| \
    (((x)>>8)&0x0000ff00)| \
    (((x)<<8)&0x00ff0000)| \
    (((x)<<24)&0xff000000))

#define SWAP_SHORT(x) \
    ((((x)>>16)&0x0000ffff)| \
    (((x)<<16)&0xffff0000))

/* start address of VME RAM (MVME162-222) */
static const u_int32_t start=0x00400000;
static const int single_am=0x9;
static const u_int32_t pattern=0x12345678;

struct quest {
    int sequence;
    int size;
    int rw;
    int mmap;
};

struct pathdescr {
    int rp, cp;
    void *map;
    size_t mapsize;
};

int controller;
u_int32_t testflags;
int do_swap;
const char *dev;
char *base;

/*****************************************************************************/
static void
usage(char* argv0)
{
    printf("usage: %s [-c] [-b] [-w] [-l num] [-t flags] [-h] device)\n", argv0);
    printf("       -w: force little endian\n");
    printf("       -m0: disable system controller (16 MHz VME clock)\n");
    printf("       -m1: enable system controller\n");
    printf("            default depends on jumper settings\n");
    printf("       -t: testflags\n");
    printf("       -h: this text\n");
    printf("       device: /dev/sis1100_xx\n");
}
/*****************************************************************************/
static int
readargs(int argc, char* argv[])
{
    int c, errflg=0;

    do_swap=0;
    controller=-1;
    testflags=0;

    while (!errflg && ((c=getopt(argc, argv, "cbwm:sl:t:h"))!=-1)) {
        switch (c) {
        case 'w':
            do_swap=1;
            break;
        case 'm':
            controller=atoi(optarg);
            break;
        case 't':
            testflags=atoi(optarg);
            break;
        case 'h':
        default:
            errflg=1;
        }
    }

    if (errflg || argc-optind!=1) {
        usage(argv[0]);
        return -1;
    }

    dev=argv[optind];

    return 0;
}
/*****************************************************************************/
static int
filter(const struct dirent *dirent)
{
    return !strncmp(dirent->d_name, base, strlen(base));
}
/*****************************************************************************/
static int
do_open(struct pathdescr *d, const char *name)
{
    struct dirent **dirents;
    char *bcopy, *dcopy;
    char *dir; /* 'base' is needed by 'filter', therefore in global scope */
    char path[PATH_MAX];
    int num, i;

    d->rp=d->cp=-1;

    /* copy name because dirname and basename may modify there arguments */
    bcopy=strdup(name);
    dcopy=strdup(name);

    /* decompose name */
    base=basename(bcopy);
    dir=dirname(dcopy);
    
    /* scan directory */
    num=scandir(dir, &dirents, filter, alphasort);
    if (num<0) {
        printf("scandir \"%s\": %s\n", base, strerror(errno));
        return -1;
    }

    /* try to open the sis devices */
    for (i=0; i<num; i++) {
        enum sis1100_subdev subdev;
        int p;

        snprintf(path, PATH_MAX-1, "%s/%s", dir, dirents[i]->d_name);
        if ((p=open(path, O_RDWR, 0))<0) {
            printf("open(\"%s\"): %s\n", path, strerror(errno));
            continue;
        }
        printf("%s open as %d\n", path, p);
        if (ioctl(p, SIS1100_DEVTYPE, &subdev)<0) {
            printf("SIS1100_DEVTYPE(%s): %s\n", path, strerror(errno));
            close(p);
            continue;
        }
        if (subdev==sis1100_subdev_remote) {
            printf("%d is 'remote'\n", p);
            d->rp=p;
        } else if (subdev==sis1100_subdev_ctrl) {
            printf("%d is 'ctrl'\n", p);
            d->cp=p;
        } else {
            printf("closing %d\n", p);
            close(p);
        }
    }

    if (d->rp<0) {
        printf("'remote' path not found\n");
        return -1;
    }
    if (d->cp<0) {
        printf("'ctrl' path not found\n");
        return -1;
    }

    for (i=0; i<num; i++)
        free(dirents[i]);
    free(dirents);

    free(bcopy);
    free(dcopy);

    return 0;
}
/*****************************************************************************/
static int
read_ctrl(struct pathdescr *d, int offs, u_int32_t *val)
{
    struct sis1100_ctrl_reg reg;

    reg.offset=offs;
    if (ioctl(d->rp, SIS1100_CTRL_READ, &reg)<0) {
        printf("CTRL_READ: %s\n", strerror(errno));
        return -1;
    } else if (reg.error) {
        printf("CTRL_READ: error=0x%x\n", reg.error);
        return -1;
    }
    *val=reg.val;
    return 0;
}
/*****************************************************************************/
static int
write_ctrl(struct pathdescr *d, u_int32_t offs, u_int32_t val)
{
    struct sis1100_ctrl_reg reg;

    reg.offset=offs;
    reg.val=val;
    if (ioctl(d->rp, SIS1100_CTRL_WRITE, &reg)<0) {
        printf("CTRL_WRITE: %s\n", strerror(errno));
        return -1;
    } else if (reg.error) {
        printf("CTRL_WRITE: error=0x%x\n", reg.error);
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static int
do_mmap(struct pathdescr *d)
{
/*
 * This only correct if start is a multiple of 0x400000!
 */
    u_int32_t descr[4]={0xff010800, single_am, start, 0};
    u_int32_t mapsize;
    int i, offs;

    if (ioctl(d->rp, SIS1100_MAPSIZE, &mapsize)<0) {
        printf("ioctl(SIS1100_MAPSIZE(vme): %s\n", strerror(errno));
        return -1;
    }
    if (mapsize<4096) {
        printf("mmap needs %u bytes but only %u are available\n",
                4096, mapsize);
        return -1;
    }

    d->mapsize=4096;

    /* fill the descriptor */
    for (i=0, offs=0x400; i<4; i++, offs+=4) {
        struct sis1100_ctrl_reg reg;
        reg.offset=offs;
        reg.val=descr[i];
        printf("map: write 0x%08x to 0x%04x\n", reg.val, reg.offset);
        if (ioctl(d->cp, SIS1100_CTRL_WRITE, &reg)<0) {
            printf("ioctl SIS1100_CTRL_WRITE: %s\n", strerror(errno));
            return -1;
        }
    }

    d->map=mmap(0, d->mapsize, PROT_READ|PROT_WRITE, MAP_SHARED,
            d->rp, 0);
    if (d->map==MAP_FAILED) {
        printf("mmap 0x%llx byte: %s\n",
                (unsigned long long)d->mapsize, strerror(errno));
        return -1;
    }

    printf("0x%llx bytes mapped at addr %p\n",
            (unsigned long long)d->mapsize, d->map);

    return 0;
}
/*****************************************************************************/
static int
prepare_settings(struct pathdescr *d)
{
    u_int32_t sc;
    int _swap=0, fifomode=0;
    if (read_ctrl(d, 0x100, &sc)<0) {
        printf("error reading OPT-VME-Master Status/Control register\n");
        return -1;
    }
    printf("system controller is %sabled\n", sc&0x10000?"en":"DIS");
    if (controller>=0) {
        if (write_ctrl(d, 0x100, 1<<(controller?0:16))<0) {
            printf("error writing OPT-VME-Master Status/Control register\n");
            return -1;
        }
        if (read_ctrl(d, 0x100, &sc)<0) {
            printf("error reading OPT-VME-Master Status/Control register\n");
            return -1;
        }
        printf("system controller is now %sabled\n", sc&0x10000?"en":"dis");
    }
    if (ioctl(d->rp, SIS1100_SWAP, &_swap)<0) {
        printf("SIS1100_SWAP: %s\n", strerror(errno));
        return -1;
    }
    if (ioctl(d->rp, SIS1100_FIFOMODE, &fifomode)<0) {
        printf("FIFOMODE: %s\n", strerror(errno));
        return -1;
    }
    if (ioctl(d->rp, SIS1100_TESTFLAGS, &testflags)<0) {
        printf("SIS1100_TESTFLAGS: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}
/*****************************************************************************/
static int
set_vmespace(struct pathdescr *d, int datasize, int am, int swap)
{
    struct vmespace vspace;

    vspace.datasize=datasize;
    vspace.am=am;
    vspace.swap = swap;
    vspace.mindmalen=0;
    vspace.mapit = 0;
#if 0
    printf("vmespace: p=%d size=%d am=%x mindmalen=%d fifo=%d\n",
        p, datasize, am, mindmalen, fifomode);
#endif
    if (ioctl(d->rp, SETVMESPACE, &vspace)<0) {
        printf("SETVMESPACE: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static int
set_vmespace_norm(struct pathdescr *d)
{
    return set_vmespace(d, 4, 9, 0);
}
/*****************************************************************************/
static void
print_quest(struct quest *quest, const char* txt)
{
    printf("[%02x] %c size=%d mmap=%d %s\n",
        quest->sequence,
        quest->rw?'W':'R',
        quest->size,
        quest->mmap,
        txt?txt:"");
    fflush(stdout);
}
/*****************************************************************************/
static void
swap_block(u_int32_t *src, u_int32_t *dst, int num, int size)
{
    int i;

    if (do_swap) {
        if (dst!=src) {
            for (i=0; i<num; i++)
                dst[i]=src[i];
        }
    } else {
        switch (size) {
        case 4:
            if (dst!=src) {
                for (i=0; i<num; i++)
                    dst[i]=src[i];
            }
            break;
        case 2:
            for (i=0; i<num; i++)
                dst[i]=SWAP_SHORT(src[i]);
            break;
        case 1:
            for (i=0; i<num; i++)
                dst[i]=SWAP_BYTES(src[i]);
            break;
        }
    }
}
/*****************************************************************************/
static int
do_write(struct pathdescr *d, struct quest *quest, int offs)
{
    set_vmespace(d, quest->size, single_am, do_swap);
    if (quest->mmap) {
        u_int8_t  *mm=(u_int8_t* )d->map+offs;
        volatile u_int32_t *m4=(u_int32_t*)mm;
        volatile u_int16_t *m2=(u_int16_t*)mm;
        volatile u_int8_t  *m1=(u_int8_t* )mm;
        switch (quest->size) {
        case 4:
            *m4=pattern&0xffffffff;
            break;
        case 2:
            *m2=pattern&0xffff;
            break;
        case 1:
            *m1=pattern&0xff;
            break;
        }
    } else {
        int res;
        res=pwrite(d->rp, &pattern, quest->size, start+offs);
        if (res!=quest->size) {
            printf("pwrite(): res=%d (%s)\n", res, strerror(errno));
            return -1;
        }
    }
    return 0;
}
/*****************************************************************************/
static int
do_write_test(struct pathdescr *d, struct quest *quest)
{
    u_int32_t block[4], xblock[4];
    u_int32_t *x4=(u_int32_t*)xblock;
    u_int16_t *x2=(u_int16_t*)xblock;
    u_int8_t  *x1=(u_int8_t* )xblock;
    int offs, res, i;

    for (offs=0; offs<4; offs+=quest->size) {

        block[0]=0x11121314;
        block[1]=0x21222324;
        block[2]=0x31323334;
        block[3]=0x41424344;

        swap_block(block, xblock, 4, quest->size);
        switch (quest->size) {
        case 4:
            x4[offs/quest->size]=pattern&0xffffffff;
            break;
        case 2:
            x2[offs/quest->size]=pattern&0xffff;
            break;
        case 1:
            x1[offs/quest->size]=pattern&0xff;
            break;
        }
        swap_block(xblock, xblock, 4, quest->size);

        set_vmespace_norm(d);
        res=pwrite(d->rp, block, 16, start);
        if (res!=16) {
            printf("pwrite(block): res=%d (%s)\n", res, strerror(errno));
            return -1;
        }

        res=do_write(d, quest, offs);
        if (res<0)
            return -1;

        set_vmespace_norm(d);
        res=pread(d->rp, block, 16, start);
        if (res!=16) {
            printf("pwrite(block): res=%d (%s)\n", res, strerror(errno));
            return -1;
        }

        res=0;
        for (i=0; i<4; i++) {
            if (block[i]!=xblock[i])
                res++;
        }
        if (res) {
            printf("offs=%d\n", offs);
            for (i=0; i<4; i++)
                printf("%08x --> %08x\n", xblock[i], block[i]);
            return -1;
        }
    }

    return 0;
}
/*****************************************************************************/
static int
do_read(struct pathdescr *d, struct quest *quest, int offs, u_int32_t *val)
{
    set_vmespace(d, quest->size, single_am, do_swap);
    if (quest->mmap) {
        u_int8_t  *mm=(u_int8_t* )d->map+offs;
        volatile u_int32_t *m4=(u_int32_t*)mm;
        volatile u_int16_t *m2=(u_int16_t*)mm;
        volatile u_int8_t  *m1=(u_int8_t* )mm;
        switch (quest->size) {
        case 4:
            *val=*m4;
            break;
        case 2:
            *val=*m2;
            break;
        case 1:
            *val=*m1;
            break;
        }
    } else {
        int res;
        *val=0;
        res=pread(d->rp, val, quest->size, start+offs);
        if (res!=quest->size) {
            printf("pread(): res=%d (%s)\n", res, strerror(errno));
            return -1;
        }
    }
    return 0;
}
/*****************************************************************************/
static int
do_read_test(struct pathdescr *d, struct quest *quest)
{
    u_int32_t block[4], xblock[4];
    u_int32_t *x4=(u_int32_t*)xblock;
    u_int16_t *x2=(u_int16_t*)xblock;
    u_int8_t  *x1=(u_int8_t* )xblock;
    u_int32_t val=0, xval=0;
    int offs, res, i;

    for (offs=0; offs<4; offs+=quest->size) {

        block[0]=0x11121314;
        block[1]=0x21222324;
        block[2]=0x31323334;
        block[3]=0x41424344;

        swap_block(block, xblock, 4, quest->size);

        set_vmespace_norm(d);
        res=pwrite(d->rp, block, 16, start);
        if (res!=16) {
            printf("pwrite(block): res=%d (%s)\n", res, strerror(errno));
            return -1;
        }

        res=do_read(d, quest, offs, &val);
        if (res<0)
            return -1;

        switch (quest->size) {
        case 4:
            xval=x4[offs/quest->size];
            break;
        case 2:
            xval=x2[offs/quest->size];
            break;
        case 1:
            xval=x1[offs/quest->size];
            break;
        }

        if (val!=xval) {
            printf("val: %08x, expected: %08x, offs=%d\n", val, xval, offs);
            for (i=0; i<4; i++)
                printf("%08x\n", block[i]);
            return -1;
        }
    }

    return 0;
}
/*****************************************************************************/
static int
do_quest(struct pathdescr *d, struct quest *quest)
{
    int res;

#if 0
    if (quest->sequence!=0x13)
        return 0;
#endif
#if 1
    if (quest->size==2)
        return 0;
#endif
#if 0
    if (!quest->rw)
        return 0;
#endif
#if 0
    if (!quest->mmap)
        return 0;
#endif
#if 0
    if (quest->rw && quest->size<4)
        return 0;
#endif


    {
        u_int32_t testflags=0;
#if 0
        if (quest->size==2)
            testflags|=1;
#endif
        if (ioctl(d->rp, SIS1100_TESTFLAGS, &testflags)<0) {
            printf("ioctl(TESTFLAGS): %s\n", strerror(errno));
            return -1;    
        }
    }

    print_quest(quest, 0);
    if (quest->rw)
        res=do_write_test(d, quest);
    else
        res=do_read_test(d, quest);

    return res;
}
/*****************************************************************************/
static int
do_tests(struct pathdescr *d)
{
    struct quest quest;
    int wordsize;
    int mmap;
    int rw;
    int sequence=0;

    for (wordsize=4; wordsize>0; wordsize>>=1) {
        for (mmap=0; mmap<=1; mmap++) {
            for (rw=0; rw<=1; rw++) {
                quest.size=wordsize;
                quest.mmap=mmap;
                quest.rw=rw;
                quest.sequence=sequence;

                if (do_quest(d, &quest)<0)
                    return -1;

                sequence++;
            }
        }
    }
    return 0;
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    struct pathdescr descr;

    if (readargs(argc, argv)<0)
        return 1;

    if (do_open(&descr, dev)<0)
        return 2;

    if (do_mmap(&descr)<0)
        return 3;

    if (prepare_settings(&descr)<0)
        return 4;

    do_tests(&descr);

    munmap(descr.map, descr.mapsize);
    close(descr.cp);
    close(descr.rp);
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/

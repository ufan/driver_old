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
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "dev/pci/sis1100_var.h"
#ifndef SIS3100_Version
#define SIS3100_Version 100
#endif


#define VMESTART 0x84000000

#if SIS3100_Version < 200
enum sis1100_subdev {sis1100_subdev_vme, sis1100_subdev_ram,
    sis1100_subdev_dsp};
#endif

struct path {
    char* name;
    enum sis1100_subdev type;
    int p;
    u_int32_t mapsize;
    u_int32_t* map;
    struct sis1100_ident ident;
} *pathes;
int numpathes;

static void
printusage(int argc, char* argv[])
{
    printf("usage: %s [-h] pathnames...\n",
        argv[0]);
}

static void
printhelp(int argc, char* argv[])
{
printf("printhelp not yet implemented\n");
}

static int
getoptions(int argc, char* argv[])
{
    extern char *optarg;
    extern int optind;
    extern int opterr;
    extern int optopt;
    int errflag, c, i;
    const char* args="h";

    optarg=0; errflag=0;
    
    while (!errflag && ((c=getopt(argc, argv, args))!=-1)) {
        switch (c) {
        case 'h': printhelp(argc, argv); break;
        default: errflag++;
        }
    }

    if (errflag || optind==argc) {
        printusage(argc, argv);
        return -1;
    }

    numpathes=argc-optind;
    pathes=malloc(numpathes*sizeof(struct path));
    for (i=0; i<numpathes; i++) {
        pathes[i].name=argv[optind+i];
    }

    return 0;
}

static int
init_path(struct path* path)
{
    path->type=-1;
    path->p=-1;
    path->mapsize=0;
    path->map=0;

    path->p=open(path->name, O_RDWR, 0);
    if (path->p<0) {
        printf("open \"%s\": %s\n", path->name, strerror(errno));
        return -1;
    }
    if (ioctl(path->p, SIS1100_DEVTYPE, &path->type)<0) {
        printf("ioctl(%s, SIS1100_DEVTYPE): %s\n",
                path->name, strerror(errno));
        return -1;
    }
    switch (path->type) {
    case sis1100_subdev_vme:
        printf("%s is VME\n", path->name);
        break;
    case sis1100_subdev_ram:
        printf("%s is RAM\n", path->name);
        break;
#if SIS3100_Version >= 200
    case sis1100_subdev_ctrl:
        printf("%s is CTRL\n", path->name);
        break;
#endif
    case sis1100_subdev_dsp:
        printf("%s is DSP\n", path->name);
        break;
    default:
        printf("init_path: %s has unknown type %d\n",
                path->name, path->type);
        return -1;
    }
    return 0;
}

static int
done_path(struct path* path)
{
    if (path->map) {
        if (munmap(path->map, path->mapsize)<0)
            printf("munmap(%s): %s\n",
                path->name, strerror(errno));
    }
    if (path->p) close(path->p);
    return 0;
}

static int
check_open(struct path* path)
{
    int p;

    /* try to open the device a second time */
    p=open(path->name, O_RDWR, 0);
#if SIS3100_Version < 200
    if (p<0) {
        printf("open \"%s\" a second time: %s\n",
                path->name, strerror(errno));
        return -1;
    }
    close(p);
#else
    if (p>=0) {
        printf("open \"%s\" a second time: success (but it should fail)\n",
                path->name);
        close(p);
        return -1;
    }
    if (errno!=EBUSY) {
        printf("open \"%s\" a second time returns \"%s\" "
                "(but EBUSY is expected)\n",
                path->name, strerror(errno));
        return -1;
    }
#endif
    return 0;
}

static int
check_RESET(struct path* path)
{
    if (path->type!=sis1100_subdev_vme) return 0;
    if (ioctl(path->p, SIS3100_RESET, &path->mapsize)<0) {
        printf("ioctl(%s, SIS3100_RESET): %s\n",
                path->name, strerror(errno));
        return -1;
    }
    return 0;
}

static int
check_MAPSIZE(struct path* path)
{
#if SIS3100_Version < 200
    switch (path->type) {
    case sis1100_subdev_vme: {
        struct sis1100_mapinfo info;
        info.space=2;
        if (ioctl(path->p, SIS1100_MAPINFO, &info)<0) {
            printf("ioctl(%s, SIS1100_MAPINFO, space=2): %s\n",
                    path->name, strerror(errno));
            printf("sizeof off_t is %d\n", sizeof(off_t));
            return -1;
        }
        path->mapsize=info.size;
        if (info.offset!=0) {
            printf("ioctl(%s, SIS1100_MAPINFO, space=2): "
                    "offset=0x%08llx (should be 0)\n",
                    path->name, info.offset);
            return -1;
        }
    }
    break;
    case sis1100_subdev_ram: {
        off_t max;
        max=lseek(path->p, 0, SEEK_END);
        if (max==(off_t)-1) {
            printf("lseek(%s, 0, SEEK_END): %s\n", path->name, strerror(errno));
            return -1;
        }
        path->mapsize=max;
    }
    break;
    /* no default */
    }
#else
    if (ioctl(path->p, SIS1100_MAPSIZE, &path->mapsize)<0) {
        printf("ioctl(%s, SIS1100_MAPSIZE): %s\n",
                path->name, strerror(errno));
        return -1;
    }
#endif

    printf("%s: mapsize=0x%x", path->name, path->mapsize);
    if (path->mapsize>=(1<<20))
        printf(" (%d MByte)", path->mapsize>>20);
    else if (path->mapsize>=(1<<10))
        printf(" (%d KByte)", path->mapsize>>10);
    printf("\n");
    switch (path->type) {
    case sis1100_subdev_vme:
        if (!path->mapsize)
            printf("%s: no map available; but no real error\n", path->name);
        else if (path->mapsize!=0x10000000) {
            printf("%s: wrong mapsize 0x%x (should be 0x10000000)\n",
                path->name, path->mapsize);
            return -1;
        }
        break;
    case sis1100_subdev_ram: break;
#if SIS3100_Version >= 200
    case sis1100_subdev_ctrl:
        if (path->mapsize!=0x1000) {
            printf("%s: unexpected mapsize 0x%x (should be 0x1000)\n",
                path->name, path->mapsize);
            return -1;
        }
        break;
#endif
    case sis1100_subdev_dsp:
        if (path->mapsize) {
            printf("%s: unexpected mapsize 0x%x (dsp can not be mapped (yet))\n",
                path->name, path->mapsize);
            return -1;
        }
        break;
    default:
        printf("check_MAPSIZE: %s has unknown type %d\n",
                path->name, path->type);
        return -1;
    }

    return 0;
}

static int
check_mmap(struct path* path)
{
#if SIS3100_Version < 200
    if (path->type!=sis1100_subdev_vme) return 0;
#endif

    if (!path->mapsize) return 0;
    path->map=mmap(0, path->mapsize, PROT_READ|PROT_WRITE,
            MAP_FILE|MAP_SHARED/*|MAP_VARIABLE*/, path->p, 0);

    if (path->map==MAP_FAILED) {
        printf("mmap(%s, 0x%x): %s\n", path->name, path->mapsize,
            strerror(errno));
        path->map=0;
    } else
        printf("%s: 0x%x Bytes mapped at %p\n", path->name, path->mapsize,
            path->map);
 
    switch (path->type) {
    case sis1100_subdev_vme:
        if (!path->map)
            printf("  Not a real error.\n");
        else
            printf("  OK.\n");
        break;
    case sis1100_subdev_ram:
        if (!path->map)
            printf("  As expected.\n");
        else {
            printf("  But that is not possible (yet).\n");
            return -1;
        }
        break;
#if SIS3100_Version >= 200
    case sis1100_subdev_ctrl:
        if (!path->map) {
            printf("  But it should work.\n");
            return -1;
        } else
            printf("  OK.\n");
        break;
#endif
    case sis1100_subdev_dsp:
        if (!path->map)
            printf("  OK.\n");
        else {
            printf("  But that is not possible (yet).\n");
            return -1;
        }
        break;
    default:
        printf("check_mmap: %s has unknown type %d\n",
                path->name, path->type);
        return -1;
    }
    return 0;
}

static void
print_ident(struct sis1100_ident_dev* ident)
{
    printf("  hw_type   =%2d ", ident->hw_type);
    switch (ident->hw_type) {
        case 1: printf(" (PCI/PLX)"); break;
        case 2: printf(" (VME)"); break;
        case 3: printf(" (CAMAC/FERA)"); break;
        case 4: printf(" (LVD/SCSI)"); break;
    }
    printf("\n");
    printf("  hw_version=%2d\n", ident->hw_version);
    printf("  fw_type   =%2d\n", ident->fw_type);
    printf("  fw_version=%2d\n", ident->fw_version);
}

static int
check_IDENT(struct path* path)
{
struct sis1100_ident {
    struct sis1100_ident_dev local;
    struct sis1100_ident_dev remote;
    int remote_ok;
    int remote_online;
};

    if (ioctl(path->p, SIS1100_IDENT, &path->ident)<0) {
        printf("ioctl(%s, SIS1100_IDENT): %s\n", path->name, strerror(errno));
        return -1;
    }    
    if (path->type==sis1100_subdev_vme) {
        printf("Local Interface:\n");
        print_ident(&path->ident.local);
        if (path->ident.remote_ok) {
            printf("Remote Interface:\n");
            print_ident(&path->ident.remote);
            printf("  remote interface o%sline\n",
                    path->ident.remote_online?"n":"ff");
        } else
            printf("no remote interface\n");
    }
    return 0;
}

static int
check_rw_single(struct path* path, u_int32_t start, u_int32_t size)
{
    u_int32_t *in, *out;
    int i, res, first, count;

    in=calloc(size, 4);
    out=calloc(size, 4);
    if (!in || !out) {
        printf("cannot allocate %d words for in- and output\n", size);
        free(in); free(out);
        return -1;
    }
    
    for (i=0; i<size; i++) {
        out[i]=random();
        in[i]=~out[i];
    }
    if (lseek(path->p, start, SEEK_SET)!=start) {
        printf("check_rw_single: lseek(%s, 0x%08x, SEEK_SET): %s\n",
                path->name, start, strerror(errno));
        return -1;
    }
    for (i=0; i<size; i++) {
        res=write(path->p, out+i, 4);
        if (res!=4) {
            printf("write(%s, ..., 4): %s\n", path->name, strerror(errno));
            return -1;
        }
    }
    if (lseek(path->p, start, SEEK_SET)!=start) {
        printf("check_rw_single: lseek(%s, 0x%08x, SEEK_SET): %s\n",
                path->name, start, strerror(errno));
        return -1;
    }
    for (i=0; i<size; i++) {
        res=read(path->p, in+i, 4);
        if (res!=4) {
            printf("read(%s, ..., 4): %s\n", path->name, strerror(errno));
            return -1;
        }
    }
    first=1; count=0;
    for (i=0; i<size; i++) {
        if (in[i]!=out[i]) {
            if (first) {
                printf("%s: rw error:", path->name);
                first=0;
            }
            printf("[%3d]: %08x --> %08x\n", i, out[i], in[i]);
            count++;
        }
    }
    return count?-1:0;
}

static int
check_rw_block(struct path* path, u_int32_t start, u_int32_t max)
{
    static u_int32_t *buf=0;
    static int bufsize=0;

    u_int32_t size;
    int i, res, first, count;

    printf("check_rw_block: max=0x%08x\n", max);
    if (bufsize<max) {
        free(buf);
        bufsize=(((max-1)>>20)+1)<<20;
        printf("\nmax=0x%x bufsize=0x%x\n", max, bufsize);
        if (bufsize<max) {
            printf("\nprogrammfehler.\n");
            return -1;
        }
        buf=malloc(bufsize*4);
        if (!buf) {
            bufsize=0;
            printf("\ncannot allocate %d words for in- and output\n", bufsize);
            return -1;
        }
    }

    for (i=0; i<max; i++) {
        buf[i]=i;
    }
    if (lseek(path->p, start, SEEK_SET)!=start) {
        printf("\nlseek(%s, 0x%08x, SEEK_SET): %s\n",
                path->name, start, strerror(errno));
        return -1;
    }
    res=write(path->p, buf, 4*max);
    if (res!=4*max) {
        printf("\nwrite(%s, ..., 4*%d): %s\n", path->name, max, strerror(errno));
        return -1;
    }
    for (i=0; i<max; i++) {
        buf[i]=~i;
    }
    for (size=1; size<=max; size++) {
        if (lseek(path->p, start, SEEK_SET)!=start) {
            printf("\nlseek(%s, 0x%08x, SEEK_SET): %s\n",
                    path->name, start, strerror(errno));
            return -1;
        }
        res=read(path->p, buf, 4*size);
        if (res!=4*size) {
            u_int32_t err;
            printf("\nread(%s, ..., 4*%d): ", path->name, size);
            if (res&3) printf("res=%d\n", res);
            if (res>=0) {
                printf("res=%d (%4d)\n", res/4, size-res/4);
            } else {
                printf("%s\n", strerror(errno));
            }
            if (ioctl(path->p, SIS1100_LAST_ERROR, &err)<0) {
                printf("\nioctl(%s, LAST_ERROR): %s\n",
                    path->name, strerror(errno));
            } else {
                if (err==0x211)
                    return 0;
                else
                    printf("prot_err: 0x%x\n", err);
            }
            return -1;
        }
        first=1; count=0;
        for (i=0; i<size; i++) {
            if (buf[i]!=i) {
                if (first) {
                    printf("\n%s: block rw error:", path->name);
                    first=0;
                }
                printf("[%3d]: %08x --> %08x\n", i, i, buf[i]);
                count++;
            }
        }
        if (count) return -1;
    }
    return 0;
}

static int
vme_probe(struct path* path, u_int32_t addr)
{
    struct vmespace space;
    int res;

    space.am=0x9;
    space.datasize=4;
    space.swap=1;
    space.mapit=0;
    space.mindmalen=-1;
    res=ioctl(path->p, SIS1100_SETVMESPACE, &space);
    if (res) {
        printf("SETVMESPACE(%s): %s\n", path->name, strerror(errno));
        return -1;
    }
    res=ioctl(path->p, SIS3100_VME_PROBE, &addr);
    if (res) {
        printf("VME_PROBE(%s, 0x%08x): %s\n", path->name, addr, strerror(errno));
    }
    return res;
}

static int
check_rw_vme(struct path* path)
{
    struct vmespace space;
    int res, size;

    printf("testing sdram over VME\n");
    if (vme_probe(path, VMESTART)) return 0;

    space.am=0x9;
    space.datasize=4;
    space.swap=1;
    space.mapit=0;
    space.mindmalen=-1;
    res=ioctl(path->p, SIS1100_SETVMESPACE, &space);
    if (res) {
        printf("SETVMESPACE(%s): %s\n", path->name, strerror(errno));
        return -1;
    }
    if (check_rw_single(path, VMESTART, 10000)<0) return -1;

    space.am=0xb;
    res=ioctl(path->p, SIS1100_SETVMESPACE, &space);
    if (res) {
        printf("SETVMESPACE(%s): %s\n", path->name, strerror(errno));
        return -1;
    }
    size=1;
    while (check_rw_block(path, VMESTART, size)>=0) {
        printf(".");
        fflush(stdout);
        size<<=4;
    }
    printf("OK.\n");
    return 0;
}

static int
check_rw_ram(struct path* path)
{
    printf("testing sdram\n");
    if (check_rw_single(path, 0, 1000)<0) return -1;
    if (check_rw_block(path, 0, 1024)<0) return -1;
    printf("OK.\n");
    return 0;
}

static int
check_rw_ctrl(struct path* path)
{
    return 0;
}

static int
check_rw_dsp(struct path* path)
{
    return 0;
}

static int
check_rw(struct path* path)
{
    switch (path->type) {
    case sis1100_subdev_vme: return check_rw_vme(path); break;
    case sis1100_subdev_ram: return check_rw_ram(path); break;
#if SIS3100_Version >= 200
    case sis1100_subdev_ctrl: return check_rw_ctrl(path); break;
#endif
    case sis1100_subdev_dsp: return check_rw_dsp(path); break;
    default:
        printf("check_rw: %s has unknown type %d\n",
                path->name, path->type);
        return -1;
    }
}

typedef int(*testfunc)(struct path*);
testfunc funcs[]={
    init_path,
    check_open,
    /*check_RESET,*/
    check_MAPSIZE,
    check_mmap, /* requires check_MAPSIZE */
    check_IDENT, /* can use check_mmap */
    check_rw,

#if 0
    check_SETVMESPACE,
    check_VME_PROBE,
    check_VME_READ,
    check_VME_WRITE,
    check_VME_BLOCK_READ,
    check_VME_BLOCK_WRITE,
    check_CONTROL_READ,
    check_CONTROL_WRITE,
    check_CONTROL_READ,
    check_CONTROL_WRITE,
    check_PIPE,
    check_LAST_ERROR,
    check_FIFOMODE,

    check_BIGENDIAN,

    check_IRQ_CTL,
    check_IRQ_GET,
    check_IRQ_ACK,
    check_IRQ_WAIT,

    check_MINDMALEN,

    check_FRONT_IO,
    check_FRONT_PULSE,
    check_FRONT_LATCH,

    check_VME_SUPER_BLOCK_READ,
    check_WRITE_PIPE,

    check_DMA_ALLOC,
    check_DMA_FREE,

    check_DUMP,
#endif
};
int numfuncs=sizeof(funcs)/sizeof(testfunc);

int main(int argc, char* argv[])
{
    int res=0, i, j;

    printf("==== SIS1100/3100 Test; V1.0 ====\n\n");

    if (getoptions(argc, argv)<0) return 1;

    srandom(17);

    for (i=0; i<numfuncs; i++) {
        for (j=0; j<numpathes; j++) {
            if (funcs[i](pathes+j)<0) {res=i+3; goto raus;}
        }
    }

raus:
    for (j=0; j<numpathes; j++) done_path(pathes+j);
    return res;
}

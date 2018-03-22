#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "../dev/pci/sis1100_var.h"

#define SWAP_BYTES(x) \
    ((((x)>>24)&0x000000ff)| \
    (((x)>>8)&0x0000ff00)| \
    (((x)<<8)&0x00ff0000)| \
    (((x)<<24)&0xff000000))

#define SWAP_SHORT(x) \
    ((((x)>>16)&0x0000ffff)| \
    (((x)<<16)&0xffff0000))

/* start address of VME RAM (MVME162-222) */
static const u_int32_t start=0x00000000;
//static const int maxwords=0x10000;
static const int maxwords=0x10000;
static const int maxdumpwords=0x20;
static const int block_am=0xb;
static const int single_am=0x9;
static const u_int32_t pattern[]={0x01234567, 0x89abcdef};
static u_int32_t *block_r; /* read data */
static u_int32_t *blocksr; /* simulated read for verify */
static u_int32_t *block_w; /* data to be written */
static u_int32_t *blockXX; /* known garbage */

struct quest {
    int sequence;
    int wordsize;
    int cpushift;
    int vmeshift;
    int dma;
    int fifo;
    int rw;
    double time;
};

int check, shift, berr, controller, loops;
u_int32_t testflags;
const char *dev;

/*****************************************************************************/
static void
usage(char* argv0)
{
    printf("usage: %s [-c] [-b] [-l num] [-t flags] [-h] device)\n", argv0);
    printf("       -c: don't check data\n");
    printf("       -b: force bus error\n");
    printf("       -m0: disable system controller (16 MHz VME clock)\n");
    printf("       -m1: enable system controller\n");
    printf("            default depends on jumper settings\n");
    printf("       -s: shift areas\n");
    printf("       -l: number of loops (0: infinite)\n");
    printf("       -t: testflags\n");
    printf("       -h: this text\n");
    printf("       device: /dev/sis1100_xxremote\n");
}
/*****************************************************************************/
static int
readargs(int argc, char* argv[])
{
    int c, errflg=0;

    check=1;
    berr=0;
    controller=-1;
    shift=0;
    loops=1;
    testflags=0;

    while (!errflg && ((c=getopt(argc, argv, "cbmsl:t:h"))!=-1)) {
        switch (c) {
        case 'c':
            check=0;
            break;
        case 'b':
            berr=1;
            break;
        case 'm':
            controller=atoi(optarg);
            break;
        case 's':
            shift=1;
            break;
        case 'l':
            loops=atoi(optarg);
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
initblocks(void)
{
    u_int32_t val;
    int i;

    block_r=(u_int32_t*)malloc(8*maxwords);
    blocksr=(u_int32_t*)malloc(8*maxwords);
    block_w=(u_int32_t*)malloc(8*maxwords);
    blockXX=(u_int32_t*)malloc(8*maxwords);
    if (!block_r || !blocksr || !block_w || !blockXX) {
        printf("malloc %d words: %s\n", 6*maxwords, strerror(errno));
        return -1;
    }

#if 0
    printf("block_r=%p\n", block_r);
    printf("blocksr=%p\n", blocksr);
    printf("block_w=%p\n", block_w);
    printf("blockXX=%p\n", blockXX);
#endif

    for (i=0; i<2*maxwords; i+=2) {
        block_w[i+0]=pattern[0];
        block_w[i+1]=pattern[1];
    }

    val=0x00010203;
    for (i=0; i<2*maxwords; i++) {
        blockXX[i]=val+i*0x10101010;
    }
    return 0;
}
/*****************************************************************************/
static int
read_ctrl(int p, int offs, u_int32_t *val)
{
    struct sis1100_ctrl_reg reg;
    int res;

    reg.offset=offs;
    if (ioctl(p, SIS1100_CTRL_READ, &reg)<0) {
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
write_ctrl(int p, u_int32_t offs, u_int32_t val)
{
    struct sis1100_ctrl_reg reg;
    int res;

    reg.offset=offs;
    reg.val=val;
    if (ioctl(p, SIS1100_CTRL_WRITE, &reg)<0) {
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
prepare_settings(int p)
{
    u_int32_t sc;
    if (read_ctrl(p, 0x100, &sc)<0) {
        printf("error reading OPT-VME-Master Status/Control register\n");
        return -1;
    }
    printf("system controller is %sabled\n", sc&0x10000?"en":"DIS");
    if (controller>=0) {
        if (write_ctrl(p, 0x100, 1<<(controller?0:16))<0) {
            printf("error writing OPT-VME-Master Status/Control register\n");
            return -1;
        }
        if (read_ctrl(p, 0x100, &sc)<0) {
            printf("error reading OPT-VME-Master Status/Control register\n");
            return -1;
        }
        printf("system controller is now %sabled\n", sc&0x10000?"en":"dis");
    }
    if (ioctl(p, SIS1100_TESTFLAGS, &testflags)<0) {
        printf("SIS1100_TESTFLAGS: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}
/*****************************************************************************/
static int
set_vmespace(int p, int datasize, int am, int mindmalen, int fifomode)
{
    struct vmespace vspace;

    vspace.datasize=datasize;
    vspace.am=am;
    vspace.swap = 0;
    vspace.mindmalen=mindmalen;
    vspace.mapit = 0;
#if 0
    printf("vmespace: p=%d size=%d am=%x mindmalen=%d fifo=%d\n",
        p, datasize, am, mindmalen, fifomode);
#endif
    if (ioctl(p, SETVMESPACE, &vspace)<0) {
        printf("SETVMESPACE: %s\n", strerror(errno));
        return -1;
    }
    if (ioctl(p, SIS1100_FIFOMODE, &fifomode)<0) {
        printf("FIFOMODE: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static void
print_quest(struct quest *quest, const char* txt)
{
    printf("[%02x] %c size=%d dma=%d",
        quest->sequence,
        quest->rw?'W':'R',
        quest->wordsize,
        quest->dma);
    if (shift) {
        printf(" cshift=%d vshift=%d",
            quest->cpushift,
            quest->vmeshift);
    }
    printf(" fifo=%d %s",
        quest->fifo,
        txt?txt:"");
    fflush(stdout);
}
/*****************************************************************************/
static double
calc_speed(struct timeval t0, struct timeval t1, int words)
{
    double diff, time;
    diff=(double)(t1.tv_sec-t0.tv_sec)*1000000.+(double)(t1.tv_usec-t0.tv_usec);
    time=diff/words;
    return time;
}
/*****************************************************************************/
static int
do_block_write(int p, struct quest *quest)
{
    char* cpuaddr;
    u_int32_t vmeaddr, val;
    struct timeval t0, t1;
    int res;


    /* write sequence marker and fill VME memory with known garbage */
    set_vmespace(p, 4, 9, 0, 0);
    val=0xffff0000+quest->sequence;
    pwrite(p, &val, 4, start);
    //set_vmespace(p, 4, 9, 0, 0);
    //pwrite(p, &val, 4, start);
    //pwrite(p, &val, 4, start);
    if (check) {
        res=pwrite(p, blockXX, 8*maxwords, start);
        if (res!=8*maxwords) {
            printf("pwrite(blockXX): res=%d (%s)\n", res, strerror(errno));
            return -1;
        }
    }

    /* do the real write */
    set_vmespace(p, quest->wordsize, quest->fifo?single_am:block_am,
            quest->dma?1:0, quest->fifo);
    cpuaddr=((char*)block_w)+quest->wordsize*quest->cpushift;
    vmeaddr=start+quest->wordsize*quest->vmeshift;
    gettimeofday(&t0, 0);
    res=pwrite(p, cpuaddr, 4*maxwords, vmeaddr);
    gettimeofday(&t1, 0);
    if (res!=4*maxwords) {
        u_int32_t prot_error=0x5a5a5a5a;
        ioctl(p, SIS1100_LAST_ERROR, &prot_error);
        printf("write: res=%d instead of %d, prot_error=%x errno=%s\n",
                res, 4*maxwords, prot_error, strerror(errno));
        return -1;
    }

    if (check) {
        /* fill readbuffer with known garbage */
        bcopy(blockXX, block_r, 8*maxwords);
        /* read the data back */
        set_vmespace(p, 4, 9, 0, 0);
        res=pread(p, block_r, 8*maxwords, start);
        if (res!=8*maxwords) {
            printf("pread: res=%d (%s)\n", res, strerror(errno));
            return -1;
        }
    }

    quest->time=calc_speed(t0, t1, maxwords);

    return 0;
}
/*****************************************************************************/
static void
simulate_block_write(struct quest *quest)
{
    int i;

    switch (quest->wordsize) {
    case 4:
        /* fill memory with known garbage */
        bcopy(blockXX, blocksr, 8*maxwords);
        if (quest->fifo) {
            if (quest->rw)
                blocksr[0]=block_w[maxwords-1];
            else
                for (i=0; i<maxwords; i++)
                    blocksr[i]=block_w[0];
        } else {
            for (i=0; i<maxwords; i++)
                blocksr[i]=block_w[i];
        }
        break;
    case 2: {
            u_int16_t *src=(u_int16_t*)block_w, *dst=(u_int16_t*)blocksr;
            int cshift=quest->cpushift, vshift=quest->vmeshift;

            bcopy(blockXX, blocksr, 8*maxwords);
            if (quest->fifo) {
                if (quest->rw) {
                    dst[vshift^1]=src[maxwords*2-1+cshift];
                } else {
                    for (i=0; i<maxwords*2; i++)
                        dst[i+cshift]=src[vshift^1];
                }
            } else {
                if (quest->rw) {
                    for (i=0; i<maxwords*2; i++)
                        dst[(i+vshift)^1]=src[i+cshift];
                } else {
                    for (i=0; i<maxwords*2; i++)
                        dst[i+cshift]=src[(i+vshift)^1];
                }
            }
        }
        break;
    case 1: {
            u_int8_t *src=(u_int8_t*)block_w, *dst=(u_int8_t*)blocksr;
            int cshift=quest->cpushift, vshift=quest->vmeshift;

            bcopy(blockXX, blocksr, 8*maxwords);
            if (quest->fifo) {
                if (quest->rw) {
                    dst[vshift^3]=src[maxwords*4-1+cshift];
                } else {
                    for (i=0; i<maxwords*4; i++)
                        dst[i+cshift]=src[vshift^3];
                }
            } else {
                if (quest->rw) {
                    for (i=0; i<maxwords*4; i++)
                        dst[(i+vshift)^3]=src[i+cshift];
                } else {
                    for (i=0; i<maxwords*4; i++)
                        dst[i+cshift]=src[(i+vshift)^3];
                }
            }
        }
        break;
    }
}
/*****************************************************************************/
static int
do_block_read(int p, struct quest *quest)
{
    char* cpuaddr;
    u_int32_t vmeaddr, val;
    struct timeval t0, t1;
    int res;

    /* write sequence marker and fill VME memory with known data */
    set_vmespace(p, 4, 9, 0, 0);
    val=0xffff0000+quest->sequence;
    res=pwrite(p, &val, 4, start);
    if (res!=4) {
        u_int32_t prot_error=0x5a5a5a5a;
        ioctl(p, SIS1100_LAST_ERROR, &prot_error);
        printf("pwrite(block_r): res=%d instead of %d, prot_error=%x "
                "errno=%s\n", res, 4, prot_error, strerror(errno));
    }
    if (check) {
        /* fill VME memory with known data */
        res=pwrite(p, block_w, 8*maxwords, start);
        if (res!=8*maxwords) {
            u_int32_t prot_error=0x5a5a5a5a;
            ioctl(p, SIS1100_LAST_ERROR, &prot_error);
            printf("pwrite(block_r): res=%d instead of %d, prot_error=%x "
                    "errno=%s\n", res, 8*maxwords,
                    prot_error, strerror(errno));
            return -1;
        }

        /* fill readbuffer with known garbage */
        bcopy(blockXX, block_r, 8*maxwords);
    }

    /* do the read */
    set_vmespace(p, quest->wordsize, block_am, quest->dma?1:0, quest->fifo);
    cpuaddr=((char*)block_r)+quest->wordsize*quest->cpushift;
    vmeaddr=start+quest->wordsize*quest->vmeshift;
    gettimeofday(&t0, 0);
    res=pread(p, cpuaddr, 4*maxwords, vmeaddr);
    gettimeofday(&t1, 0);
    if (res!=4*maxwords) {
        u_int32_t prot_error=0x5a5a5a5a;
        ioctl(p, SIS1100_LAST_ERROR, &prot_error);
        printf("read: res=%d instead of %d, prot_error=%x errno=%s\n",
                res, 4*maxwords, prot_error, strerror(errno));
        return -1;
    }

    quest->time=calc_speed(t0, t1, maxwords);

    return 0;
}
/*****************************************************************************/
static int
do_check(struct quest *quest)
{
    int i, failed=0;

    if (quest->rw)
        simulate_block_write(quest);
    else
        //simulate_block_read(quest);
        simulate_block_write(quest);
    for (i=0; i<2*maxwords; i++) {
        if (blocksr[i]!=block_r[i])
            failed++;
    }
    if (failed) {
        printf("mismatch in %d word%s\n", failed, failed==1?"":"s");
        for (i=0; i<maxdumpwords; i++) {
            if (blocksr[i]!=block_r[i])
                printf("[%d] %08x --> %08x%s\n", i, blocksr[i], block_r[i],
                        blocksr[i]==block_r[i]?"":" !");
        }
        if (quest->wordsize==2) {
            u_int16_t *blocksr16=( u_int16_t*)blocksr;
            u_int16_t *block_r16=( u_int16_t*)block_r;
            u_int16_t *block_w16=( u_int16_t*)block_w;
            for (i=0; i<maxdumpwords; i++) {
                if (blocksr16[i]!=block_r16[i])
                    printf("[%02d] %04x --> %04x  %04x%s\n", i,
                            blocksr16[i], block_r16[i],
                            block_w16[i],
                            blocksr16[i]==block_r16[i]?"":" !");
            }
        }
        if (quest->wordsize==1) {
            u_int8_t *blocksr8=( u_int8_t*)blocksr;
            u_int8_t *block_r8=( u_int8_t*)block_r;
            u_int8_t *block_w8=( u_int8_t*)block_w;
            for (i=0; i<maxdumpwords; i++) {
                if (blocksr8[i]!=block_r8[i])
                    printf("[%02d] %02x --> %02x  %02x%s\n", i,
                            blocksr8[i], block_r8[i],
                            block_w8[i],
                            blocksr8[i]==block_r8[i]?"":" !");
            }
        }
    }

    return failed?-1:0;
}
/*****************************************************************************/
static int
do_block(int p, struct quest *quest)
{
    int res;

    print_quest(quest, 0);
    if (quest->rw)
        res=do_block_write(p, quest);
    else
        res=do_block_read(p, quest);
    if (!res)
        printf("  %6.3f us/word\n", quest->time);
    else
        printf("\n");

    return res;
}
/*****************************************************************************/
static int
do_quest(int p, struct quest *quest)
{
#if 0
    if (quest->sequence!=0x64)
        goto skipped;
#endif
#if 0
    if (quest->wordsize!=2)
        goto skipped;
#endif
#if 0
    if (!quest->rw)
        goto skipped;
#endif
#if 0
    if (!quest->dma)
        goto skipped;
#endif
#if 0
    if (quest->fifo)
        goto skipped;
#endif
#if 1
    if (quest->dma && quest->fifo)
        goto skipped;
#endif
#if 0
    if (quest->rw && quest->wordsize<4)
        goto skipped;
#endif
#if 0
    if (!shift && quest->vmeshift)
        goto skipped;
#endif
#if 0
    if (!shift && quest->cpushift)
        goto skipped;
#endif

    if (!shift && (quest->vmeshift || quest->cpushift))
        goto skipped;

    if (do_block(p, quest)<0)
        return -1;
    if (check) {
        if (do_check(quest)<0)
            return -1;
    }

    return 0;

skipped:
    //print_quest(quest, "skipped");
    return 0;
}
/*****************************************************************************/
static int
test_block(int p)
{
    struct quest quest;
    int wordsize;
    int cpushift;
    int vmeshift;
    int dma;
    int fifo;
    int rw;
    int sequence=0;

    for (wordsize=4; wordsize>0; wordsize>>=1) {
        for (cpushift=0; cpushift<=4-wordsize; cpushift++) {
            for (vmeshift=0; vmeshift<=4-wordsize; vmeshift++) {
                for (dma=0; dma<=1; dma++) {
                    for (fifo=0; fifo<=1; fifo++) {
                        for (rw=0; rw<=1; rw++) {
                            quest.wordsize=wordsize;
                            quest.cpushift=cpushift;
                            quest.vmeshift=vmeshift;
                            quest.dma=dma;
                            quest.fifo=fifo;
                            quest.rw=rw;
                            quest.sequence=sequence;
                            
                            if (do_quest(p, &quest)<0)
                                return -1;

                            sequence++;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    int p, l;

    if (readargs(argc, argv)<0)
        return 1;

    if ((p=open(dev, O_RDWR, 0))<0) {
        printf("open \"%s\": %s\n", dev, strerror(errno));
        return 2;
    }

    if (prepare_settings(p)<0)
        return 3;

    if (initblocks()<0)
        return 4;

    for (l=0; l<loops || !loops; l++) {
        printf("loop %d\n", l);
        if (test_block(p))
            break;
    }

    close(p);
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/

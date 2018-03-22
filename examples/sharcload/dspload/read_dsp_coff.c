#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>

#ifndef MAP_VARIABLE
#define MAP_VARIABLE 0x0000
#endif

#include "dspcode.h"

struct opaque_info {
    void *addr;
    size_t len;
    int path;
};

struct filehdr
{
    u_int16_t   f_magic;        /* magic number */
    u_int16_t   f_nscns;        /* number of sections */
    int32_t     f_timdat;       /* time & date stamp */
    int32_t     f_symptr;       /* file pointer to symtab */
    int32_t     f_nsyms;        /* number of symtab entries */
    u_int16_t   f_opthdr;       /* sizeof(optional hdr) */
    u_int16_t   f_flags;        /* flags */
};

#define SYMNMLEN 8
struct secthdr
{
    char        s_name[SYMNMLEN];   /* section name */
    int32_t     s_paddr;            /* physical address */
    int32_t     s_vaddr;            /* virtual address */
    int32_t     s_size;             /* section size */
    int32_t     s_scnptr;           /* file ptr to raw data for section */
    int32_t     s_relptr;           /* file ptr to relocation */
    int32_t     s_lnnoptr;          /* file ptr to line numbers */
    u_int16_t   s_nreloc;           /* number of relocation entries */
    u_int16_t   s_nlnno;            /* number of line number entries */
    int32_t     s_flags;            /* type and content flags */
};
/* section header flags */
#define SECTION_PM          0x00000001L
#define SECTION_DM          0x00000002L
#define SECTION_RAM	    0x00000004L
#define SECTION_ROM         0x00000008L
#define SECTION_16          0x00000010L
#define SECTION_32          0x00000020L
#define SECTION_40          0x00000040L
#define SECTION_48          0x00000080L

static int
free_dsp_coff(struct code_info* info)
{
    struct opaque_info* info_=info->opaque_info;
    munmap(info_->addr, info_->len);
    close(info_->path);
    free(info_);
    free(info->sections);
    return 0;
}

size_t
read_dsp_coff(int p, const char* codepath, u_int8_t** code,
        struct code_info* info)
{
    struct opaque_info* info_;
    struct section* sections;
    struct filehdr* filehdr;
    struct stat stat;
    u_int8_t *mp;
    struct tm *tm;
    char time[1024];
    time_t tt;
    int i;

    info->free=0;
    info->sections=0;
    info->nsect=0;

    if (fstat(p, &stat)<0) {
        printf("cannot stat \"%s\": %s\n", codepath, strerror(errno));
        close(p);
        return -1;
    }

    mp=mmap(0, stat.st_size, PROT_READ,
            MAP_VARIABLE|MAP_FILE|MAP_SHARED, p, 0);
    if (mp==MAP_FAILED) {
        printf("cannot mmap \"%s\": %s\n", codepath, strerror(errno));
        close(p);
        return -1;
    }

    filehdr=(struct filehdr*)mp;
    if (filehdr->f_magic!=0x521c) {
        printf("bad magic number 0x%x\n", filehdr->f_magic);
        munmap(mp, stat.st_size);
        close(p);
        return -1;
    }

    tt=filehdr->f_timdat;
    tm=localtime(&tt);
    strftime(time, 1024, "%Y-%m-%d %T %Z", tm);
    printf("timestamp: %s\n", time);

    sections=malloc(filehdr->f_nscns*sizeof(struct section));
    if (!sections) {
        printf("malloc info for %d sections: %s\n",
                filehdr->f_nscns, strerror(errno));
        munmap(mp, stat.st_size);
        close(p);
        return -1;
    }

    for (i=0; i<filehdr->f_nscns; i++) {
        struct secthdr* secthdr=(struct secthdr*)(mp+
                                sizeof(struct filehdr)+
                                filehdr->f_opthdr+
                                i*sizeof(struct secthdr));
        sections[i].is_code=0;
        printf("section %d: %s ,code %x, size %d, addr=0x%x)\n",
                i,
                secthdr->s_name,
                secthdr->s_flags,
                secthdr->s_size,
                secthdr->s_paddr);

        sections[i].s_name=secthdr->s_name;
        sections[i].s_flags=secthdr->s_flags;
        sections[i].s_size=secthdr->s_size;
        sections[i].s_paddr=secthdr->s_paddr;
        sections[i].addr=mp+secthdr->s_scnptr;

        if ((secthdr->s_flags==(SECTION_PM|SECTION_48)) && (secthdr->s_size)) {
            sections[i].is_code=1;
        }
    }
    *code=0;

    info_=malloc(sizeof(struct opaque_info));
    if (!info_) {
        printf("malloc info (%lld bytes): %s\n",
                (unsigned long long)(sizeof(struct opaque_info)),
                strerror(errno));
        munmap(mp, stat.st_size);
        close(p);
        free(sections);
        return -1;
    }
    info_->addr=mp;
    info_->len=stat.st_size;
    info_->path=p;
    info->opaque_info=info_;
    info->free=free_dsp_coff;
    info->sections=sections;
    info->nsect=filehdr->f_nscns;

    return 0;
}

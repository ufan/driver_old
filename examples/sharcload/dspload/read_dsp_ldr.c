#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "dspcode.h"

struct opaque_info {
    u_int8_t *addr;
};

static int
free_dsp_ldr(struct code_info* info)
{
    struct opaque_info* info_=info->opaque_info;
    free(info_->addr);
    free(info_);
    return 0;
}

size_t
read_dsp_ldr(int p, const char* codepath, u_int8_t** code,
        struct code_info* info)
{
    struct stat stat;
    int arrsize;
    FILE* fp;
    u_int8_t* code_;
    struct opaque_info* info_;
    size_t size;
    int lc, i;

    info->free=0;

    if (fstat(p, &stat)<0) {
        printf("cannot stat \"%s\": %s\n", codepath, strerror(errno));
        close(p);
        return -1;
    }

    fp=fdopen(p, "r");
    if (!fp) {
        printf("fdopen \"%s\": %s\n", codepath, strerror(errno));
        close(p);
        return -1;
    }

    /* grobe schaetzung */
    arrsize=(stat.st_size*2)/7;
    arrsize+=arrsize/2;

    code_=malloc(arrsize);
    if (!code_) {
        printf("malloc code (%d bytes): %s\n", arrsize, strerror(errno));
        fclose(fp);
        return -1;
    }

    size=0; lc=0;
    do {
        unsigned int w;
        char line[1024];
        char *end;
        int l;

        lc++;
        if (!fgets(line, 1024, fp)) break;
        l=strlen(line);
        if (!l) continue;
        if (line[l-1]=='\n') {
            l--;
            line[l]='\0';
        }
        if (line[l-1]=='\r') {
            l--;
            line[l]='\0';
        }

        w=strtoul(line, &end, 0);
        if (*end!='\0') {
            printf("conversion error in line %d: %s\n", lc, line);
            break;
        }

        if (size+1>=arrsize) {
            /* realloc ... */
            printf("too many bytes\n");
            free(code_);
            fclose(fp);
            return -1;
        }
        code_[size++]=(w>>8)&0xff;
        code_[size++]=w&0xff;
    } while (1);

    fclose(fp);

    for (i=0; i<size; i+=6) {
        u_int16_t* w=( u_int16_t*)(code_+i);
        u_int16_t ww;
        ww=w[2];
        w[2]=w[0];
        w[0]=ww;
    }

    *code=code_;

    info_=malloc(sizeof(struct opaque_info));
    if (!info_) {
        printf("malloc info (%lld bytes): %s\n",
                (unsigned long long)(sizeof(struct opaque_info)),
                strerror(errno));
        free(code_);
        return -1;
    }
    info_->addr=code_;
    info->opaque_info=info_;
    info->free=free_dsp_ldr;

    return size;
}

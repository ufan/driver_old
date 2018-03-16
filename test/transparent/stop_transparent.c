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

#include <sis1100_var.h>

struct pathdescr {
    int dp, cp;
};

static const char *dev;
static char *base;

/*****************************************************************************/
static void
usage(char* argv0)
{
    printf("usage: %s device)\n", argv0);
    printf("       device: /dev/sis1100_xx\n");
}
/*****************************************************************************/
static int
readargs(int argc, char* argv[])
{
    int c, errflg=0;

    while (!errflg && ((c=getopt(argc, argv, "h"))!=-1)) {
        switch (c) {
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

    d->dp=d->cp=-1;

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
        if (ioctl(p, SIS1100_DEVTYPE, &subdev)<0) {
            printf("SIS1100_DEVTYPE(%s): %s\n", path, strerror(errno));
            close(p);
            continue;
        }
        if (subdev==sis1100_subdev_dsp)
            d->dp=p;
        else if (subdev==sis1100_subdev_ctrl)
            d->cp=p;
        else
            close(p);
    }

    if (d->dp<0) {
        printf("'dsp' path not found\n");
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
#if 0
static int
prepare_settings(struct pathdescr *d)
{
    u_int32_t v;

    v=1;
    if (ioctl(d->cp, SIS1100_TRANSPARENT, &v)<0) {
        printf("SIS1100_TRANSPARENT: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}
#endif
/*****************************************************************************/
static int
reset_settings(struct pathdescr *d)
{
    u_int32_t v;

    v=0;
    if (ioctl(d->cp, SIS1100_TRANSPARENT, &v)<0) {
        printf("SIS1100_TRANSPARENT: %s\n", strerror(errno));
        return -1;
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

#if 0
    if (prepare_settings(&descr)<0)
        return 3;

    do {
        u_int32_t val;
        int special, num, i;

        res=read(descr.dp, &val, 4);
        if (res!=4) {
            printf("read: res=%d errno=%s\n", res, strerror(errno));
            break;
        }
        num     = val&0x7fffffff;
        special = val&0x80000000;
        printf("%d %s\n", num, special?"special":"data");
        for (i=0; i<num; i++) {
            res=read(descr.dp, &val, 4);
            if (res!=4) {
                printf("read: res=%d errno=%s\n", res, strerror(errno));
                break;
            }
            printf("  0x%08x\n", val);
        }
    } while (1);
#endif
    if (reset_settings(&descr)<0)
        return 3;

    close(descr.cp);
    close(descr.dp);
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/

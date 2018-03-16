/* $ZEL$ */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <dirent.h>

#include <sis1100_var.h>

/****************************************************************************/
static int
filter(const struct dirent *dirent)
{
    int is_sis;
    int is_ctrl;
    is_sis=!strncmp(dirent->d_name, "sis1100_", 8);
    if (!is_sis)
        return 0;
    is_ctrl=!strncmp(dirent->d_name+9, "0ctrl", 5);
    return is_ctrl;
}
/****************************************************************************/
static char *boardtypes[]= {
    "5V",
    "3.3V",
    "universal",
    "PCIe+OPT",
    "PCIe single link",
    "PCIe quad link",
};
static char *hwtypes[]= {
    "invalid",
    "pci",
    "vme",
    "camac",
    "lvd",
    "pandapixel",
};
static int
do_sis_id(const struct dirent *dirent)
{
    struct sis1100_ident ident;
    u_int32_t ser[4];
    int p;

    printf("%s:\n", dirent->d_name);
    if ((p=open(dirent->d_name, O_RDWR, 0))<0) {
        fprintf(stderr, "open(\"%s\"): %s\n", dirent->d_name, strerror(errno));
        return -1;
    }

    if (ioctl(p, SIS1100_IDENT, &ident)<0) {
	fprintf(stderr, "ioctl(SIS1100_IDENT): %s\n", strerror(errno));
	return -1;
    }
    if (ioctl(p, SIS1100_SERIAL_NO, ser)<0) {
	fprintf(stderr, "ioctl(SIS1100_SERIAL_NO): %s\n", strerror(errno));
	return -1;
    }
    
    printf("local part:\n");
    printf("  hw_type   : %d\n", ident.local.hw_type);
    printf("  hw_version: %d\n", ident.local.hw_version);
    printf("  fw_type   : %d\n", ident.local.fw_type);
    printf("  fw_version: %d\n", ident.local.fw_version);
    printf("  board type: %d, ", ser[0]);
    if (ser[0]<sizeof(boardtypes)/sizeof(char*))
        printf("%s\n", boardtypes[ser[0]]);
    else
        printf("(unknown)\n");
    printf("  serial    : %d\n", ser[1]);


    if (ident.remote_ok) {
        printf("remote part:\n");
	printf("  hw_type   : %d, ", ident.remote.hw_type);
        if (ser[0]<sizeof(hwtypes)/sizeof(char*))
            printf("%s\n", hwtypes[ident.remote.hw_type]);
        else
            printf("(unknown)\n");
	printf("  hw_version: %d\n", ident.remote.hw_version);
	printf("  fw_type   : %d\n", ident.remote.fw_type);
	printf("  fw_version: %d\n", ident.remote.fw_version);
    } else {
    	printf("remote id not available\n");
    }

    printf("\n");

    close(p);
    return 0;
}
/****************************************************************************/
int
main(int argc, const char *argv[])
{
    const char *base="/dev";
    struct dirent **dirents;
    int num, i;

    if (argc>1)
        base=argv[1];

    num=scandir(base, &dirents, filter, alphasort);
    if (num<0) {
        fprintf(stderr, "scandir \"%s\": %s\n", base, strerror(errno));
        return 1;
    }

    if (chdir(base)<0) {
        fprintf(stderr, "chdir \"%s\": %s\n", base, strerror(errno));
        return 1;
    }

    for (i=0; i<num; i++)
        do_sis_id(dirents[i]);

    return 0;
}
/****************************************************************************/
/****************************************************************************/

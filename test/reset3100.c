#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <dev/pci/sis1100_var.h>

int main(int argc, char* argv[])
{
    int p;
    struct sis1100_ident ident;

    if (argc<2)
      {
      fprintf(stderr, "usage: %s path\n", argv[0]);
      return 1;
      }

    if ((p=open(argv[1], O_RDWR, 0))<0)
      {
      fprintf(stderr, "open(\"%s\"): %s\n", argv[1], strerror(errno));
      return 1;
      }

    if (ioctl(p, SIS3100_RESET)<0) {
	fprintf(stderr, "ioctl(SIS3100_RESET): %s\n", strerror(errno));
	return 1;
    }

    if (ioctl(p, SIS1100_IDENT, &ident)<0) {
	fprintf(stderr, "ioctl(SIS1100_IDENT): %s\n",strerror(errno));
	return 1;
    }

    
    printf("local  hw_type   : %d\n", ident.local.hw_type);
    printf("local  hw_version: %d\n", ident.local.hw_version);
    printf("local  fw_type   : %d\n", ident.local.fw_type);
    printf("local  fw_version: %d\n\n", ident.local.fw_version);
    if (ident.remote_ok<0) {
    	printf("remote id not available\n");
    } else {
	printf("remote hw_type   : %d\n", ident.remote.hw_type);
	printf("remote hw_version: %d\n", ident.remote.hw_version);
	printf("remote fw_type   : %d\n", ident.remote.fw_type);
	printf("remote fw_version: %d\n\n", ident.remote.fw_version);
        printf("remote side is %sonline\n", ident.remote_online?"":"not ");
    }

    close(p);
    return 0;
}

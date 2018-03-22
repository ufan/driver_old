/* $ZEL: ident.c,v 1.3 2010/08/02 19:25:39 wuestner Exp $ */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <dev/pci/sis1100_var.h>

static u_int32_t read_remote_register(int p, u_int32_t offs)
{
    struct sis1100_ctrl_reg reg;

    reg.offset=offs;
    if (ioctl(p, SIS3100_CONTROL_READ, &reg)<0) {
	fprintf(stderr, "ioctl(SIS3100_CONTROL_READ, offs=0x%x): errno=%s\n",
    	    offs, strerror(errno));
	return -1;
    }
    if (reg.error) {
	fprintf(stderr, "ioctl(SIS3100_CONTROL_READ, offs=0x%x): error=%d\n",
    	    offs, reg.error);
	return -1;
    }
    return reg.val;
}

static u_int32_t read_local_register(int p, u_int32_t offs)
{
    struct sis1100_ctrl_reg reg;

    reg.offset=offs;
    if (ioctl(p, SIS1100_CONTROL_READ, &reg)<0) {
	fprintf(stderr, "ioctl(SIS1100_CONTROL_READ, offs=0x%x): errno=%s\n",
    	    offs, strerror(errno));
	return -1;
    }
    if (reg.error) {
	fprintf(stderr, "ioctl(SIS1100_CONTROL_READ, offs=0x%x): error=%d\n",
    	    offs, reg.error);
	return -1;
    }
    return reg.val;
}

static u_int32_t read_ident(int p, struct sis1100_ident* ident)
{
    if (ioctl(p, SIS1100_IDENT, ident)<0) {
	fprintf(stderr, "ioctl(SIS1100_IDENT): %s\n",strerror(errno));
	return -1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    int p;
    u_int32_t id;
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

    id=read_local_register(p, 0);
    printf("local id        : 0x%08x\n\n", id);

    read_ident(p, &ident);
    printf("local hw_type   : %d\n", ident.local.hw_type);
    printf("local hw_version: %d\n", ident.local.hw_version);
    printf("local fw_type   : %d\n", ident.local.fw_type);
    printf("local fw_version: %d\n\n", ident.local.fw_version);

    id=read_remote_register(p, 0);
    printf("remote id       : 0x%08x\n\n", id);

    if (!ident.remote_ok) {
    	printf("remote id not available\n");
    } else {
	printf("remote hw_type   : %d\n", ident.remote.hw_type);
	printf("remote hw_version: %d\n", ident.remote.hw_version);
	printf("remote fw_type   : %d\n", ident.remote.fw_type);
	printf("remote fw_version: %d\n\n", ident.remote.fw_version);
    }

    close(p);
    return 0;
}

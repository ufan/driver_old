#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <dev/pci/sis1100_var.h>

static u_int32_t write_local_register(int p, u_int32_t offs, u_int32_t val)
{
    struct sis1100_ctrl_reg reg;

    reg.offset=offs;
    reg.val=val;
    if (ioctl(p, SIS1100_CONTROL_WRITE, &reg)<0) {
	fprintf(stderr, "ioctl(SIS1100_CONTROL_WRITE, offs=0x%x): %s\n",
    	    offs, strerror(errno));
	return -1;
    }
    return 0;
}

static u_int32_t read_local_register(int p, u_int32_t offs)
{
    struct sis1100_ctrl_reg reg;

    reg.offset=offs;
    if (ioctl(p, SIS1100_CONTROL_READ, &reg)<0) {
	fprintf(stderr, "ioctl(SIS1100_CONTROL_READ, offs=0x%x): %s\n",
    	    offs, strerror(errno));
	return -1;
    }
    return reg.val;
}

int main(int argc, char* argv[])
{
    int p;
    u_int32_t status, control;

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

    status=read_local_register(p, 4);
    control=read_local_register(p, 8);
    printf("  before reset:\n");
    printf("status=0x%08x, control=0x%08x\n", status, control);
    write_local_register(p, 8, 1);
    status=read_local_register(p, 4);
    control=read_local_register(p, 8);
    printf("  after reset:\n");
    printf("status=0x%08x, control=0x%08x\n", status, control);

    close(p);
    return 0;
}

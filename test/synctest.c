#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <dev/pci/sis1100_var.h>

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
    int p, count;
    u_int32_t status, old_status;

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

    old_status=read_local_register(p, 4);
    printf("status=0x%08x\n", old_status);
    count=0;
    while (count++<1000000) {
    	status=read_local_register(p, 4);
	if (status!=old_status) {
	    printf("       0x%08x\n", status);
	    old_status=status;
	}
    }

    close(p);
    return 0;
}

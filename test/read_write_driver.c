#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "dev/pci/sis1100_var.h"

int mod_base=0xe00000;
int p;

/****************************************************************************/
static void printerror(struct sis1100_vme_req* req, int _errno, int write)
{
if (write)
    printf("vme write 0x%08x to 0x%08x", req->data, req->addr);
else
    printf("vme read 0x%08x", req->addr);

printf(": %s", strerror(_errno));
if (_errno==EIO) printf("; protocoll error 0x%x", req->error);
printf("\n");
}
/****************************************************************************/
int main(int argc, char* argv[])
{
struct sis1100_vme_req req;

if (argc<2)
  {
  fprintf(stderr, "usage: %s path\n", argv[0]);
  return 1;
  }
if ((p=open(argv[1], O_RDWR, 0))<0) return 1;

req.size=4;
req.am=0x39;
req.addr=mod_base+0x2020;
if (ioctl(p, SIS3100_VME_READ, &req)<0) {
    printf("SIS3100_VME_READ(2020): %s\n", strerror(errno));
    return 1;
}
printf("*0x2020=0x%08x; error=0x%x\n", req.data, req.error);

close(p);
return 0;
}

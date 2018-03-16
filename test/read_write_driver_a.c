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
int i;
struct sis1100_vme_req req;
u_int32_t base;

if (argc<2)
  {
  fprintf(stderr, "usage: %s path\n", argv[0]);
  return 1;
  }
if ((p=open(argv[1], O_RDWR, 0))<0) return 1;

req.size=2; /* driver does not change any field except data */
req.am=0x9; /* "" */

printf("search for CAEN-Modules:\n");
for (i=0, base=0; i<10; i++, base+=0x10000) {
    req.addr=base+0xfa;
    if (ioctl(p, SIS3100_VME_READ, &req)<0) {
    	printerror(&req, errno, 0);
    } else {
    	printf("data=0x%08x\n", req.data);
    	if (req.data==0xfaf5) { /* CAEN-Module */
	    u_int16_t type, serial;
	    req.addr=base+0xfc;
	    if (ioctl(p, SIS3100_VME_READ, &req)<0) {
    	    	printerror(&req, errno, 0); return 1;
	    }
	    type=req.data;
	    req.addr=base+0xfe;
	    if (ioctl(p, SIS3100_VME_READ, &req)<0) {
    	    	printerror(&req, errno, 0); return 1;
	    }
	    serial=req.data;
	    printf("at 0x%x: type=0x%x; serial=%d\n", base, type, serial&0xfff);
	}
    }
}

close(p);
return 0;
}

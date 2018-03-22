/************************************/
/* Clear CAMAC Inhibit with SIS5100 */
/****:*******************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#include "dev/pci/sis1100_var.h"
#include "sis5100_camac_calls.h"


/****************************************************************************/
int main(int argc, char* argv[])
{

int p;
int res;


if (argc<2) {
  fprintf(stderr, "usage: %s  path\n", argv[0]);
  return 1;
}

if ((p=open(argv[1], O_RDWR, 0))<0) {
	perror("open");
	return 1;
}


res=s5100_control_write(p,0x100,0x10000);
if (res!=0) {
   printf("error: %x\n",res);
}
else {
   printf("CAMAC Inhibit cleared\n");
} 
close(p);
return 0;
}





 





























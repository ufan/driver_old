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
#include "sis3100_vme_calls.h"




/****************************************************************************/
/****************************************************************************/
int main(int argc, char* argv[])
{

int p;

u_int32_t offset ; 
u_int32_t data ;

int return_code ;

if (argc<4)
  {
  fprintf(stderr, "usage: %s path  addr  data \n", argv[0]);
  return 1;
  }

if ((p=open(argv[1], O_RDWR, 0))<0) {
	perror("open");
	return 1;
}

offset     = strtoul(argv[2],NULL,0) ;
data       = strtoul(argv[3],NULL,0) ;

  return_code =  s3100_control_write(p, offset, data) ; 
  printf("s3100_control_write:   return_code = 0x%08x\n", return_code );         



close(p);
return 0;
}





 




























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
int mod_base, no_of_data, i;
u_int32_t data ;
int return_code ;

if (argc<4)
  {
  fprintf(stderr, "usage: %s path  VME_BASE_ADDR  NO_OF_DATAs  VME_WRITE_DATA  \n", argv[0]);
  return 1;
  }

if ((p=open(argv[1], O_RDWR, 0))<0) {
	perror("open");
	return 1;
}

mod_base   = strtoul(argv[2],NULL,0) ;
no_of_data = strtoul(argv[3],NULL,0) ;
data       = strtoul(argv[4],NULL,0) ;


 for(i=0;i<no_of_data;i++) { ;
   return_code =  vme_A32D32_write(p, mod_base+(4*i), data ) ;
 }

   printf("vme_A32D32_write:   return_code = 0x%08x\n", return_code );


close(p);
return 0;
}


































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
int main(int argc, char* argv[])
{
  struct sis1100_vme_req req;

int p;
int mod_base;
u_int32_t data ;
int return_code ;
int loop_counter ;
int nof_loops ;


if (argc<3)
  {
  fprintf(stderr, "usage: %s  path  VME_BASE_ADDR   LOOP_COUNTER \n", argv[0]);
  return 1;
  }

if ((p=open(argv[1], O_RDWR, 0))<0) {
	perror("open");
	return 1;
}

mod_base     = strtoul(argv[2],NULL,0) ;

loop_counter  = 0 ;
nof_loops  = 1 ;
if (argc>3) nof_loops  = strtoul(argv[3],NULL,0) ;


do {

  req.size=4; /* driver does not change any field except data */
  req.am=0x2f; /* "" */
  req.addr= mod_base;
  if (ioctl(p, SIS3100_VME_READ, &req)<0)   return -1;        /* NEW */
  return_code = req.error;
  data = req.data;    /* NEW */
  
   if (return_code != 0) {break;}
  loop_counter = loop_counter + 1 ;
 } while (loop_counter < nof_loops ) ;


printf("vme_A32D32_read:   return_code = 0x%08x      loops = 0x%08x\n", return_code , loop_counter);
printf("vme_A32D32_read:   data = 0x%08x\n", data );


close(p);
return 0;
}





 





























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

int p;
int irq_level;
u_int8_t data_8 ;
int return_code ;
int loop_counter ;


if (argc<3)
  {
  fprintf(stderr, "usage: %s  path  IRQ_LEVEL   LOOP_COUNTER \n", argv[0]);
  return 1;
  }

if ((p=open(argv[1], O_RDWR, 0))<0) {
	perror("open");
	return 1;
}


/* open VME */
/*   if ((p=open("/tmp/sis1100", O_RDWR, 0))<0) {     */
/*     printf("error on opening VME environment (/tmp/sis1100)\n");  */
/*     return -1;  */
/*   }           */

irq_level     = strtoul(argv[2],NULL,0) ;

loop_counter  = 1 ;
if (argc>3) loop_counter  = strtoul(argv[3],NULL,0) ;


 
do {
  return_code =  vme_IACK_D8_read(p, irq_level, &data_8 ) ;
  loop_counter = loop_counter - 1 ;
 } while (loop_counter != 0 ) ;

printf("vme_A32D32_read:   return_code = 0x%08x\n", return_code );
printf("vme_A32D32_read:   irq vector  = 0x%02x\n", data_8 );


close(p);
return 0;
}





 





























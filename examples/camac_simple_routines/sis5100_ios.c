/***************/
/* SIS5100 IOS */
/***************/

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
int datum;


if (argc<2) {
  fprintf(stderr, "usage: %s  path\n", argv[0]);
  return 1;
}

if ((p=open(argv[1], O_RDWR, 0))<0) {
	perror("open");
	return 1;
}


while(1){
  /* enable LAM station N1-N4 */
  res=s5100_control_write(p,0x108,0xF);
  if (res!=0) {
    printf("error: %x\n",res);
    return -1;
  }
  /* read input status */
  res=s5100_control_read(p,0x84,&datum);
  printf("OPT-IN register contents: %8.8x\n",datum); 
  sleep(1);
  res=s5100_control_write(p,0x80,0x70);
  /* read latched inputs */
  res=s5100_control_read(p,0x84,&datum);
  printf("OPT-IN IRQ register contents: %8.8x\n",datum);
  /* clear latched bits */
  res=s5100_control_write(p,0x84,datum);
  /* and read again */
  res=s5100_control_read(p,0x84,&datum);
  printf("OPT-IN IRQ register contents after clear: %8.8x\n",datum);
  res=s5100_control_read(p,0x80,&datum);
  printf("OPT-IN register contents: %8.8x\n",datum);
  sleep(1); 
  res=s5100_control_write(p,0x80,0x700000);
  res=s5100_control_read(p,0x80,&datum);
  printf("OPT-IN register contents: %8.8x\n",datum);
  /* one shot IRQ LAM update */
  res=s5100_control_write(p,0x104,0x8000);
  /* read LAM status register */
  res=s5100_control_read(p,0x104,&datum);
  printf("LAM register contents: %8.8x\n",datum);

}

close(p);
return 0;
}





 





























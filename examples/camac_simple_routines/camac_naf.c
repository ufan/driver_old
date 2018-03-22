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
u_int16_t A,N,F;
int Q=0,X=0;
u_int32_t datum ;
int res;

if (argc<6) {
  fprintf(stderr, "usage: %s  path  N A F datum\n", argv[0]);
  return 1;
}

if ((p=open(argv[1], O_RDWR, 0))<0) {
	perror("open");
	return 1;
}

N     = strtoul(argv[2],NULL,0) ;
A     = strtoul(argv[3],NULL,0) ;
F     = strtoul(argv[4],NULL,0) ;
datum     = strtoul(argv[5],NULL,0) ;

printf("Station: %d  Address: %d  Function: %d\n",N,A,F);


if (F>15) {
   res=camac_write(p,N,A,F,datum);
   Q=(~res&0x80)>>7;
   X=(~res&0x40)>>6;
}
else {
   res=camac_read(p,N,A,F,&datum);
   Q=(datum&0x80000000)>31;
   X=(datum&0x40000000)>30;
   datum=datum&0xFFFFFF;
}

if (res&~0x2c0) {
    printf("camac NAF: error=0x%x\n", res);
    return -1;
}

printf("NAF:   datum = 0x%04x\n", (int) datum );
printf("Q: %d X: %d\n",Q,X);
close(p);
return 0;
}





 





























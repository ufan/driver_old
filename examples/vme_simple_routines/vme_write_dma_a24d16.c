/* 15.02.08 DMA write based on Martin Toklien's example */
/* 27.02.08 SIS1100-eCMC                                */
/* 10.06.08 driver 2.11                                 */

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

#define MAX_NUMBER_WORDS 0x100000  

int p;
int mod_base;
int no_of_words ;
int i,j;

u_int16_t blt_data[MAX_NUMBER_WORDS] ;
u_int32_t put_words ;
int return_code ;
int loop_counter ;


if (argc<4)
  {
  fprintf(stderr, "usage: %s PATH  VME_BASE_ADDR  NO_OF_WORDS [LOOP_COUNTER]  \n", argv[0]);
  return 1;
  }

if ((p=open(argv[1], O_RDWR, 0))<0) {
     printf("no device \n"); 
     	return 1;
}

mod_base     = strtoul(argv[2],NULL,0) ;
no_of_words  = strtoul(argv[3],NULL,0) ;


loop_counter  = 1 ;
if (argc>4) loop_counter  = strtoul(argv[4],NULL,0) ;


if (no_of_words > MAX_NUMBER_WORDS)
  {
     printf("no_of_words (0x%08x) must be lower then MAX_NUMBER_WORDS (0x%08x)\n",no_of_words, MAX_NUMBER_WORDS);
     return -1;
   }

/* make sure word count is even */
no_of_words &= 0xFFFFFFFE;

/* test write data fill */
j=0;
for (i=0; i < no_of_words/2; i++) {
    blt_data[i]= j + ((j+1)<<16);
   j+=2;
   printf("j: %d\n",j);
   if (j>=15) {
      j=0;
   }
   printf("blt_data[%d]: %08x\n",i,blt_data[i]);
}

  
for (j=0; j < loop_counter; j++) {
   return_code =   vme_A24DMA_D16_write(p, mod_base, blt_data, no_of_words, &put_words) ;

   printf("vme_A24DMA_D16_write:   return_code = 0x%08x\n", return_code );
   printf("vme_A24DMA_D16_write:   put_words  = 0x%08x\n", put_words );

   for (i=0;(i)<put_words/2;i++)
    {
      printf("i = 0x%08x     Data = 0x%04x\n",i, blt_data[i]);
    }
   /* sleep(1); */
 }

close(p);
return 0;
}


/* 15.02.08 DMA read based on Martin Toklien's example   */
/* 10.06.08 driver 2.11                                  */

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

int vme_A24DMA_D16_read(int p, u_int32_t vme_addr, u_int32_t *local_addr,
			 u_int32_t wc, u_int32_t *gotwords);



/****************************************************************************/
int main(int argc, char* argv[])
{

#define MAX_NUMBER_LWORDS 0x100000  

int p;
int mod_base;
int no_of_lwords ;
int i;

u_int32_t blt_data[MAX_NUMBER_LWORDS] ;
u_int32_t get_lwords ;
int return_code ;
int loop_counter ;


if (argc<4)
  {
  fprintf(stderr, "usage: %s PATH  VME_BASE_ADDR  NO_OF_LWORDS [LOOP_COUNTER]  \n", argv[0]);
  return 1;
  }

if ((p=open(argv[1], O_RDWR, 0))<0) {
     printf("no device \n"); 
     	return 1;
}

mod_base     = strtoul(argv[2],NULL,0) ;
no_of_lwords = strtoul(argv[3],NULL,0) ;


loop_counter  = 1 ;
if (argc>4) loop_counter  = strtoul(argv[4],NULL,0) ;


if (no_of_lwords > MAX_NUMBER_LWORDS)
  {
     printf("no_of_lwords (0x%08x) must be lower then MAX_NUMBER_LWORDS (0x%08x)\n",no_of_lwords, MAX_NUMBER_LWORDS);
     return -1;
   }

/* do not allow odd word counts */
no_of_lwords &= 0xFFFFFFFE;
  
for (i=0; i < loop_counter; i++) {
    printf("before read\n");
    return_code =   vme_A24DMA_D16_read(p, mod_base, blt_data, no_of_lwords, &get_lwords) ;
    printf("after read\n");
 }



   printf("vme_A24DMA_D16_read:   return_code = 0x%08x\n", return_code );
   printf("vme_A24DMA_D16_read:   get_lwords  = 0x%08x\n", get_lwords );

   for (i=0;(i)<get_lwords;i++)
    {
      printf("i = 0x%08x     Data = 0x%08x\n",i, blt_data[i]);
    }




close(p);
return 0;
}



int vme_A24DMA_D16_read(int p, u_int32_t vme_addr, u_int32_t *local_addr,
                      u_int32_t wc, u_int32_t *gotwords)
{
struct sis1100_vme_block_req block_req;
	
	u_int16_t	*buffer;
	u_int32_t	i;
	
	int err;

	buffer = malloc( 2 * sizeof( u_int16_t ) * wc );
	if (buffer==0) return -1;
	
	block_req.num=wc;
	block_req.fifo=0;
	block_req.size=2;
	block_req.am=0x39; /*A24 */
	/* block_req.am=0x9; */ /* A32 */
	block_req.addr=vme_addr ;
	block_req.data = ( u_int8_t * )buffer ;

	err = ioctl(p, SIS3100_VME_BLOCK_READ, &block_req);
	
	if (err<0)
	{
		free( buffer );
		return -1 ;
	}
	
	for (i=0; i<block_req.num; i++) local_addr[i] = buffer[i];
	
	*gotwords = block_req.num;
	free(buffer);
	return(block_req.error); 
}

 































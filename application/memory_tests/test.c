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



/* 
int soll_data ;
int max_lword_cnt ;
int loop_cnt ;

int error_cnt ;
*/




static int getopts(int iargc, char* iargv[])
{

   extern char *optarg ;
   extern int optind ;

   const char* optstring="a:f:p";
   int c, errflag = 0 ;
   int  return_code ;

   printf("iargc  = 0x%08x\n", iargc );
   printf(" \n")  ;

   return_code = -1 ;

while (!errflag && ((c = getopt(iargc, iargv, optstring)) != -1)) {
       switch (c) {
         case 'a':
                     printf("-a iargc = 0x%08x\n", iargc );
                     printf("-a c     = 0x%08x\n", c );
                     printf("-a       = 0x%08x\n", (strtoul(optarg,NULL,0)) );
                     return_code = 0 ;
   printf(" \n")  ;
               break ;
         case 'f':
                     printf("-f iargc = 0x%08x\n", iargc );
                     printf("-f c     = 0x%08x\n", c );
                     printf("-f       = 0x%08x\n", (strtoul(optarg,NULL,0)) );
                     return_code = 0 ;
   printf(" \n")  ;
               break ;
        }   
}
return return_code ;

}


/****************************************************************************/

int main(int argc, char* argv[])
{

#define MAX_NUMBER_LWORDS 0x100000  

int i;

int mod_base;
int no_of_lwords ;
int p;
u_int32_t blt_data[MAX_NUMBER_LWORDS] ;
u_int32_t put_lwords ;
u_int32_t start_data ;

int return_code ;
int loop_counter ;

 getopts(argc,argv) ;

if (argc<4)
  {
  fprintf(stderr, "usage: %s  VME_BASE_ADDR  NO_OF_LWORDS  START_DATA [LOOP_COUNTER]  \n", argv[0]);
  return 1;
  }

/* open VME */
   if ((p=open("/tmp/sis1100", O_RDWR, 0))<0) {
     printf("error on opening VME environment\n");
     return -1;
   }

mod_base     = strtoul(argv[1],NULL,0) ;
no_of_lwords = strtoul(argv[2],NULL,0) ;
start_data   = strtoul(argv[3],NULL,0) ;


loop_counter  = 1 ;
if (argc>3) loop_counter  = strtoul(argv[4],NULL,0) ;


if (no_of_lwords > MAX_NUMBER_LWORDS)
  {
     printf("no_of_lwords (0x%08x) must be lower then MAX_NUMBER_LWORDS (0x%08x)\n",no_of_lwords, MAX_NUMBER_LWORDS);
     return -1;
   }



/* test pattern (increment ) */
   for (i=0; i<no_of_lwords; i++)   blt_data[i] = start_data + i ;

   for (i=0; i < loop_counter; i++) {
      return_code =   vme_A32BLT32_write(p, mod_base, blt_data, no_of_lwords, &put_lwords) ;
   }

   printf("vme_A32BLT32_write:   return_code = 0x%08x\n", return_code );
   printf("vme_A32BLT32_write:   put_lwords  = 0x%08x\n", put_lwords ); 



close(p);
return 0;
}





 




























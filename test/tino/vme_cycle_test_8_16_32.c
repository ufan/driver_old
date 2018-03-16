/***************************************************************************/
/*                                                                         */
/*  Filename: vme_cycle_test_8_16_32                                       */
/*                                                                         */
/* Description:                                                            */
/*                                                                         */
/*      Linux             +                                                */ 
/*                                                                         */
/*  Autor:                TH                                               */
/*  date:                 10.05.2002                                       */
/*  last modification:    11.11.2008                                       */
/*                                                                         */
/* ----------------------------------------------------------------------- */
/*                                                                         */
/*  SIS  Struck Innovative Systeme GmbH                                    */
/*                                                                         */
/*  Harksheider Str. 102A                                                  */
/*  22399 Hamburg                                                          */
/*                                                                         */
/*  Tel. +49 (0)40 60 87 305 0                                             */
/*  Fax  +49 (0)40 60 87 305 20                                            */
/*                                                                         */
/*  http://www.struck.de                                                   */
/*                                                                         */
/*  � 2008                                                                 */
/*                                                                         */
/***************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define USE_POSIX199309
#include <time.h>

#include "dev/pci/sis1100_var.h"
#include "sis3100_vme_calls.h"

#include "kbhit.h"



/*===========================================================================*/
/* Globals					  			     */
/*===========================================================================*/

#define MAX_NUMBER_OF_PRINTS 0x10      
#define MAX_NUMBER_LWORDS 0x1000000       /* 64MByte */

u_int32_t wblt_data[MAX_NUMBER_LWORDS] ;
u_int32_t rblt_data[MAX_NUMBER_LWORDS] ;

u_int32_t test1_data[16] ;




/*===========================================================================*/
/* Prototypes					  			     */
/*===========================================================================*/









/*===========================================================================*/
/* Main      					  	                     */
/*===========================================================================*/

int main(int argc, char* argv[])
{

#define MAX_NUMBER_LWORDS 0x1000000  

int p;



u_int32_t data;
unsigned int i;

unsigned int addr   ;
unsigned int no_of_lwords ;
unsigned int result ;
unsigned int loop_counter ;


int return_code ;

unsigned int  loop_cnt, error_cnt ;
char line_in[128];


int sis_ret ;


u_int32_t mod_base ;
u_int32_t test1_data[16] ;
u_int32_t test2_data[16] ;

u_int16_t temp_16 ;
u_int8_t  temp_8 ;



test1_data[0]   =   0x12345678 ;
test1_data[1]   =   0x87654321 ;
test1_data[2]   =   0x11224488 ;
test1_data[3]   =   0x88442211 ;
test1_data[4]   =   0xAA559966 ;
test1_data[5]   =   0x66AA5599 ;
test1_data[6]   =   0x9966AA55 ;
test1_data[7]   =   0x559966AA ;
test1_data[8]   =   0xffffffff ;
test1_data[9]   =   0x00000000 ;
test1_data[10]  =   0xFF00FF00 ;
test1_data[11]  =   0x00FF00FF ;
test1_data[12]  =   0xFFFF0000 ;
test1_data[13]  =   0x0000FFFF ;
test1_data[14]  =   0xF0F0F0F0 ;
test1_data[15]  =   0x0F0F0F0F ;

test2_data[0]   =   0xffffffff ;
test2_data[1]   =   0x00000000 ;
test2_data[2]   =   0xFF00FF00 ;
test2_data[3]   =   0x00FF00FF ;
test2_data[4]   =   0xFFFF0000 ;
test2_data[5]   =   0x0000FFFF ;
test2_data[6]   =   0xF0F0F0F0 ;
test2_data[7]   =   0x0F0F0F0F ;
test2_data[8]   =   0x12345678 ;
test2_data[9]   =   0x87654321 ;
test2_data[10]  =   0x11224488 ;
test2_data[11]  =   0x88442211 ;
test2_data[12]  =   0xAA559966 ;
test2_data[13]  =   0x66AA5599 ;
test2_data[14]  =   0x9966AA55 ;
test2_data[15]  =   0x559966AA ;



if (argc<4)
  {
  fprintf(stderr, "usage: %s PATH  VME_BASE_ADDR  NO_OF_LWORDS [LOOP_COUNTER]  \n", argv[0]);
  return 1;
  }

if ((p=open(argv[1], O_RDWR, 0))<0) {
      printf("error opening device\n");
      return 1;
   }

/* open VME */
/*
   if ((p=open("/tmp/sis1100", O_RDWR, 0))<0) {
     printf("error on opening VME environment\n");
     return -1;
   }
*/
mod_base     = strtoul(argv[2],NULL,0) ;
no_of_lwords = strtoul(argv[3],NULL,0) ;


loop_counter  = 1 ;
if (argc>4) loop_counter  = strtoul(argv[4],NULL,0) ;


if (no_of_lwords > MAX_NUMBER_LWORDS)
  {
     printf("no_of_lwords (0x%08x) must be lower then MAX_NUMBER_LWORDS (0x%08x)\n",no_of_lwords, MAX_NUMBER_LWORDS);
     return -1;
   }





	printf( "VME Memory Test with Single Cycle; write/read with D8, D16, D32");
	printf("\n\nstart test on address %x over %x  lwords \n\n\n\n", mod_base, no_of_lwords );
   



/******************************************************************/
/*                                                                */
/* Test VME RAM from PCI                    */
/*                                                                */
/******************************************************************/



error_cnt = 0 ;
loop_cnt  = 0 ;

do {

//printf("\n");
/* 1. Test:   Write D32 - Read D32 Test */
   addr=mod_base ;
   printf("* WD32");fflush(stdout);
   for (i=0; i<no_of_lwords; i++)  {
      return_code = vme_A32D32_write(p, addr, test1_data[i&0xf] ) ;
      if(return_code != 0) { printf("vme_A32D32_write: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}
      addr = addr + 4;
    }


   addr=mod_base ;
   printf("-RD32 ");fflush(stdout);
   for (i=0; i<no_of_lwords; i++)  {
      return_code = vme_A32D32_read(p, addr, &data ) ;
      if (return_code != 0) { printf("vme_A32D32_read: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}
      if (data != test1_data[i&0xf]) {
          printf("Test1: i = 0x%08x  written = 0x%08x   read = 0x%08x\n",i,test1_data[i&0xf],data);   
	  error_cnt = error_cnt + 1;
        }
      addr = addr + 4;
    }




/* 2. Test:   Write D32 - Read D16 Test */
   addr=mod_base ;
   printf("* WD32");fflush(stdout);
   for (i=0; i<no_of_lwords; i++)  {
      return_code = vme_A32D32_write(p, addr, test2_data[i&0xf] ) ;
      if(return_code != 0) { printf("vme_A32D32_write: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}
      addr = addr + 4;
    }


   addr=mod_base ;
   printf("-RD16 ");fflush(stdout);
   for (i=0; i<no_of_lwords; i++)  {
      return_code = vme_A32D16_read(p, addr, &temp_16 ) ;
      if (return_code != 0) { printf("vme_A32D16_read: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}
      data = (u_int32_t)temp_16 ; 
 
      addr = addr + 2;
      return_code = vme_A32D16_read(p, addr, &temp_16 ) ;
      if (return_code != 0) { printf("vme_A32D16_read: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}
      data = (data<<0x10)  ; 
      data = data + (((u_int32_t)temp_16) & 0xffff) ; 
      if (data != test2_data[i&0xf]) {
          printf("Test2: i = 0x%08x  written = 0x%08x   read = 0x%08x\n",i,test2_data[i&0xf],data);   
	  error_cnt = error_cnt + 1;
        }
      addr = addr + 2;
    }






/* 3. Test:   Write D32 - Read D8 Test */
   addr=mod_base ;
   printf("* WD32");fflush(stdout);
   for (i=0; i<no_of_lwords; i++)  {
      return_code = vme_A32D32_write(p, addr, test1_data[i&0xf] ) ;
      if(return_code != 0) { printf("vme_A32D32_write: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}
      addr = addr + 4;
    }


   addr=mod_base ;
   printf("-RD8 ");fflush(stdout);
   for (i=0; i<no_of_lwords; i++)  {
      return_code = vme_A32D8_read(p, addr, &temp_8 ) ;
      if (return_code != 0) { printf("vme_A32D8_read: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}
      data = (u_int32_t)temp_8 ; 
 
      addr = addr + 1;
      return_code = vme_A32D8_read(p, addr, &temp_8 ) ;
      if (return_code != 0) { printf("vme_A32D8_read: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}
      data = (data<<0x8)  ; 
      data = data + (((u_int32_t)temp_8) & 0xff) ; 

      addr = addr + 1;
      return_code = vme_A32D8_read(p, addr, &temp_8 ) ;
      if (return_code != 0) { printf("vme_A32D8_read: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}
      data = (data<<0x8)  ; 
      data = data + (((u_int32_t)temp_8) & 0xff) ; 

      addr = addr + 1;
      return_code = vme_A32D8_read(p, addr, &temp_8 ) ;
      if (return_code != 0) { printf("vme_A32D8_read: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}
      data = (data<<0x8)  ; 
      data = data + (((u_int32_t)temp_8) & 0xff) ; 
 

      if (data != test1_data[i&0xf]) {
          printf("Test3: i = 0x%08x  written = 0x%08x   read = 0x%08x\n",i,test1_data[i&0xf],data);   
	  error_cnt = error_cnt + 1;
        }
      addr = addr + 1;
    }






/* 4. Test:   Write D16 - Read D32 Test */
   addr=mod_base ;
   printf("* WD16"); fflush(stdout);
   for (i=0; i<no_of_lwords; i++)  {
      temp_16 = (u_int16_t)(test2_data[i&0xf] >> 0x10 );
      /*  printf("test_data = 0x%08x   temp_16_1 = 0x%04x\n",test2_data[i&0xf], temp_16); */  
      return_code = vme_A32D16_write(p, addr, temp_16 ) ;
      if(return_code != 0) { printf("vme_A32D16_write: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}

      addr = addr + 2;
      temp_16 = (u_int16_t)test2_data[i&0xf] ;
      /*  printf("test_data = 0x%08x   temp_16_2 = 0x%04x\n",test2_data[i&0xf], temp_16);   */
      return_code = vme_A32D16_write(p, addr, temp_16 ) ;
      if(return_code != 0) { printf("vme_A32D16_write: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}
      addr = addr + 2;
    }


   addr=mod_base ;
   printf("-RD32 ");fflush(stdout);
   for (i=0; i<no_of_lwords; i++)  {
      return_code = vme_A32D32_read(p, addr, &data ) ;
      if (return_code != 0) { printf("vme_A32D32_read: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}
      if (data != test2_data[i&0xf]) {
          printf("Test4: i = 0x%08x  written = 0x%08x   read = 0x%08x\n",i,test2_data[i&0xf],data);   
	  error_cnt = error_cnt + 1;
        }
      addr = addr + 4;
    }





/* 5. Test:   Write D8 - Read D32 Test */
   addr=mod_base ;
   printf("* WD8");fflush(stdout);
   for (i=0; i<no_of_lwords; i++)  {
      temp_8 = (u_int8_t)(test1_data[i&0xf] >> 0x18 );
     /*   printf("test_data = 0x%08x   temp_8_1 = 0x%02x\n",test1_data[i&0xf], temp_8);  */ 
      return_code = vme_A32D8_write(p, addr, temp_8 ) ;
      if(return_code != 0) { printf("vme_A32D8_write: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}

      addr = addr + 1;
      temp_8 = (u_int8_t)(test1_data[i&0xf] >> 0x10 );
     /*   printf("test_data = 0x%08x   temp_8_2 = 0x%02x\n",test1_data[i&0xf], temp_8); */   
      return_code = vme_A32D8_write(p, addr, temp_8) ;
      if(return_code != 0) { printf("vme_A32D8_write: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}

      addr = addr + 1;
      temp_8 = (u_int8_t)(test1_data[i&0xf] >> 0x8 );
    /*    printf("test_data = 0x%08x   temp_8_3 = 0x%02x\n",test1_data[i&0xf], temp_8);   */
      return_code = vme_A32D8_write(p, addr, temp_8) ;
      if(return_code != 0) { printf("vme_A32D8_write: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}

      addr = addr + 1;
      temp_8 = (u_int8_t) test1_data[i&0xf] ;
   /*     printf("test_data = 0x%08x   temp_8_4 = 0x%02x\n",test1_data[i&0xf], temp_8);   */
      return_code = vme_A32D8_write(p, addr, temp_8) ;
      if(return_code != 0) { printf("vme_A32D8_write: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}
      addr = addr + 1;

    }


   addr=mod_base ;
   printf("-RD32 ");fflush(stdout);
   for (i=0; i<no_of_lwords; i++)  {
      return_code = vme_A32D32_read(p, addr, &data ) ;
      if (return_code != 0) { printf("vme_A32D32_read: return_code = 0x%08x  at addr.= 0x%08x\n",return_code,addr); return 1;}
      if (data != test1_data[i&0xf]) {
          printf("Test5: i = 0x%08x  written = 0x%08x   read = 0x%08x\n",i,test1_data[i&0xf],data);   
	  error_cnt = error_cnt + 1;
        }
      addr = addr + 4;
    }


	loop_cnt++;
	printf("  loop counter =  %d    error counter = %d \n",loop_cnt,error_cnt );

} while (error_cnt==0) ;



   /* close VME environment */
   close(p);
   printf( "\n\nEnter character to leave program");
   result = scanf( "%s", line_in );

   return 0;
}











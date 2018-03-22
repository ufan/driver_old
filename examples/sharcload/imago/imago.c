/*===========================================================================*/
/*                                                                           */
/* File:             imago.c (IMAGO code)                                        */
/*                                                                           */
/* OS:               LINUX (Kernel >= 2.4.4                                  */
/*                                                                           */
/* Description:      Imago test code                                         */
/*                                                                           */
/* Version:          1.0                                                     */
/*                                                                           */
/*                                                                           */
/* Generated:        13.05.02 from rss.c                                     */
/*                                                                           */
/* Author:           MKI (Dr. Matthias Kirsch)                               */
/*                                                                           */
/* Last Change:      xx.xx.02 MKI     Installation                           */
/*---------------------------------------------------------------------------*/
/* SIS GmbH                                                                  */
/* Harksheider Str. 102A                                                     */
/* 22399 Hamburg                                                             */
/*                                                                           */
/* http://www.struck.de                                                      */
/*                                                                           */
/*===========================================================================*/

#define _GNU_SOURCE

/*===========================================================================*/
/* Headers								     */
/*===========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

/* sis1100/3100 PCI to VME specific */   
#include "dev/pci/sis1100_var.h"
#include "sis3100_calls/sis3100_vme_calls.h" 
/* SIS9200 DSP */
#include "header/sis9200.h"
#include "header/imago.h"
    

/*===========================================================================*/
/* Prototypes					  			     */
/*===========================================================================*/

int reset_dsp(void);
int load_dsp(char* dsp_path);
int read_imago_conf(char* conf_path);
int init_ddg(void);

/*===========================================================================*/
/* Global variables                                                          */
/*===========================================================================*/
int p,p_sharc,p_sdram;

/*===========================================================================*/
/* Main      					  	                     */
/*===========================================================================*/
int main(int argc, char* argv[]) {
int rc;
u_int32_t blt_data[655360];
u_int32_t readadd,data;
int retcode;
int i;
 int written;
FILE *diskfile;
 int blocks=0;

   if ((rc=read_imago_conf(CONFPATH)) != SUCCESS) {
     printf("error on reading configuration file\n");
     exit(-1);
   }

   /* open VME */ 
   if ((p=open("/tmp/sis1100", O_RDWR, 0))<0) {
     printf("error on opening VME environment\n");
     return -1;
   }
   /* open SHARC */
   if ((p_sharc=open("/tmp/sis3100sharc", O_RDWR, 0))<0) {
     printf("error on sharc open");
     return -1;
   }
   /* open SDRAM */
   if ((p_sdram=open("/tmp/sis3100sdram", O_RDWR, 0))<0) {
     printf("error on sdram open");
     return -1;
   }

   if ((rc=reset_dsp()) != SUCCESS) {
     printf("error on resetting DSP\n");
     exit(-1);
   }

   if ((rc=init_ddg()) != SUCCESS) {
     printf("error on DDG init\n");
     exit(-1);
   }


   if ((rc=load_dsp(dsppath)) != SUCCESS) {
     printf("error on loading DSP code\n");
     exit(-1);
   }


   /* main DAQ loop  */
   printf("main loop\n");
   diskfile=fopen("diskfile","w");
   system("date");
   while (blocks < 500) {
       if ((retcode = s3100_sharc_read(p_sharc, 0x40000000, &data, 0x1))!=WORDWRITTEN) {
	   printf("error on SHARC read\n");
       } 
       if (data!=0) {
	   blocks++; 
	  data*=5;   /* 5 long words per event */
          printf("bank 0 flag: %d\n",data);
          readadd=0;
          rc = s3100_sdram_read(p_sdram, readadd, blt_data, (u_int32_t)data) ;
          if (rc!=data*4) {
                printf("error on SDRAM read %08x\n",rc);
          }
          for (i=0;i<5;i++) {
              printf("blt0 data: 0x%8x \n",blt_data[i]);
	  }
          written=fwrite(blt_data,1024,2048,diskfile);
          printf("written: %d block: %d\n",written, blocks);
          data=0;
          if ((retcode = s3100_sharc_write(p_sharc, 0x40000000, &data, 0x1))!=WORDWRITTEN) {
	     printf("error on SHARC write\n");
          }
       }
       if ((retcode = s3100_sharc_read(p_sharc, 0x40000004, &data, 0x1))!=WORDWRITTEN) {
	   printf("error on SHARC read\n");
       }
       if (data!=0) {
	   blocks++; 
	  data*=5;   /* 5 long words per event */
          printf("bank 1 flag: %d\n",data);
          readadd=0x400000;
          rc = s3100_sdram_read(p_sdram, readadd, blt_data, (u_int32_t)data) ;
          if (rc!=data*4) {
                printf("error on SDRAM read %08x\n",rc);
          }
          for (i=0;i<5;i++) {
              printf("blt1 data: 0x%8x \n",blt_data[i]);
	  }         
           written=fwrite(blt_data,1024,2048,diskfile);
          printf("written: %d\n",written);

	  data=0x0; 
          if ((retcode = s3100_sharc_write(p_sharc, 0x40000004, &data, 0x1))!=WORDWRITTEN) {
	     printf("error on SHARC write\n");
          }
       }
   }
   system("date");
   fclose(diskfile);
   /* close VME environment */
   close(p);
   /* close DSP */
   close(p_sharc);
   /* close SDRAM */
   close(p_sdram);
   return 0;
}
/*===========================================================================*/
/* Read Configuration File   					  	     */
/*===========================================================================*/
int read_imago_conf(char* conf_path){
    int rc=0;
    char line_in[128];
    char *buffer;
    char *rest;
    FILE *handle;
        printf("Read configuration file: %s \n",conf_path);	
        if ((handle = fopen(conf_path,"r"))==NULL) {
	     printf("file open error: %s\n",conf_path);
             return -1;
	}

	 while (fgets(line_in,128,handle)!=NULL) {
	      if ( line_in[0] != '#')
		  { 
		    if (buffer = (char *)strtok(line_in,">"))
			   rest = (char *)strtok(NULL,">");
			if (rest)  /* not an array */
			{
                            if (!strcmp(buffer, "DSPPATH")) {
                               sscanf(rest,"%s",dsppath);
 			       printf("DSP path: %s\n",dsppath);
			    }
			}  	   
			else 	   /* is an array */
			{
			   /* not used for the time being */
			}  
			
		  }
	   }
	   fclose(handle);
	   return rc;
}
/*===========================================================================*/
/* Load DSP     					  		     */
/*===========================================================================*/

int load_dsp(char* dsppath){
    int rc=0,retcode=1;
    int count=0,loadcount=0;
    int offset;
    int currentaddress ;
    char line_in[128];
    FILE *loaderfile;
    unsigned int tempword[0x10000];
    u_int32_t data ;
    u_int32_t addr ;

    loaderfile=fopen(dsppath,"r");
    if (loaderfile>0) {
       printf("loader file %s opened\n",dsppath);
       while (retcode>0) {
          tempword[count]= strtoul(line_in,NULL,16); 
	  retcode=fscanf(loaderfile,"0x%4x\n",&tempword[count]); 
          if (count<0x10000) {
             count++;
	  }
          else {
	     printf("load file size too big\n");
             return -1;
	  }
      }
      printf("load file length: %d\n",count);
    
    }
    else {
      printf("loader file %s not found\n",dsppath);
      return -1;
    }
    fclose(loaderfile);

    offset =  0x00000300 ;
    printf("loading SHARC DSP\n");

    currentaddress=SHARCRAM;
    while (loadcount<count) {  
       addr = D48REG;
       data = tempword[loadcount];

       if ((retcode = s3100_sharc_write(p_sharc, addr, &data, 0x1))!=WORDWRITTEN) { 
	   printf("s3100_sharc_write:   retcode = 0x%08x\n", retcode ); 
           return -1;
       }
       loadcount++;

       addr = currentaddress;
       data = ((tempword[loadcount+1] << 16 ) & 0xFFFF0000) + (tempword[loadcount] & 0x0000FFFF);

       if ((retcode = s3100_sharc_write(p_sharc, addr, &data, 0x1)) != WORDWRITTEN) {
          printf("s3100_sharc_write:   retcode = 0x%08x\n", retcode );
          return -1;
       }         
       currentaddress+=4;
       loadcount+=2;
    }
    /* start SHARC */
    printf("starting SHARC DSP\n");
    if ((retcode = s3100_control_write(p, offset, 0x0100)) != SUCCESS) {    
       printf("s3100_control_write:   retcode = 0x%08x\n", retcode );
       return -1;
    }       
    /* sleep during SDRAM init */
    sleep(5);  
 return rc;
}


/*===========================================================================*/
/* reset DSP   			     		                             */
/*===========================================================================*/
int reset_dsp(void) {
    int rc=0,retcode;
    int offset;
    /* Reset SHARC */
    printf("resetting SHARC DSP\n");
    offset =  0x00000300 ;
    if ((retcode = s3100_control_write(p, offset, 0x00000800)) != SUCCESS) {
	printf("s3100_control_write:   retcode = 0x%08x\n", retcode ); 
        return -1;
    } 
    if ((retcode = s3100_control_write(p, offset, 0x01000000)) != SUCCESS) {
	printf("s3100_control_write:   retcode = 0x%08x\n", retcode ); 
        return -1;
    } 
   return rc;
}

/*===========================================================================*/
/* init DDG   			     		                             */
/*===========================================================================*/
int init_ddg(void) {
    int rc=0;
#define HITDELAY 3000
    /* set V680 to +0.4V trigger levels */
    rc+=vme_A16D16_write(p,0xc108,0x85);
    /* reset hits and gates on V680 */
    rc+=vme_A16D16_write(p,0xc110,0x3ff);

    /* disarm DDG */
    rc+=vme_A16D16_write(p,0xc010,0x8000);
    /* VOUT to TDC NIM 0 to -2.0V */
    rc+=vme_A16D16_write(p,0xc008,0x0);
    rc+=vme_A16D16_write(p,0xc00a,0xff00);
    rc+=vme_A16D16_write(p,0xc00e,0x0);
    /* set internal timer rate */
    /* rc+=vme_A16D16_write(p,0xc012,0x1); */
    rc+=vme_A16D16_write(p,0xc012,50);
    /* set wave 12 */
    rc+=vme_A16D16_write(p,0xc038,0x0);
    /* set wave 34 */
    rc+=vme_A16D16_write(p,0xc03a,0x0);
    /* set wave 56 */
    rc+=vme_A16D16_write(p,0xc03c,0x0);
    /* set delay 1 */
    rc+=vme_A16D16_write(p,0xc020,0x0);
    rc+=vme_A16D16_write(p,0xc022,0x0);
    /* set delay 3 */
    rc+=vme_A16D16_write(p,0xc028,0x0);
    rc+=vme_A16D16_write(p,0xc02a,HITDELAY);
    /* set delay 4 */
    rc+=vme_A16D16_write(p,0xc02c,0x0);
    rc+=vme_A16D16_write(p,0xc02e,HITDELAY);
    /* set delay 5 */
    rc+=vme_A16D16_write(p,0xc030,0x0);
    rc+=vme_A16D16_write(p,0xc032,HITDELAY);
    /* set delay 6 */
    rc+=vme_A16D16_write(p,0xc034,0x0);
    rc+=vme_A16D16_write(p,0xc036,HITDELAY);
    /* actions */
    rc+=vme_A16D16_write(p,0xc00c,0x81);
    /* set ITRIG */
    rc+=vme_A16D16_write(p,0xc010,0x400);  

    return rc;
}





















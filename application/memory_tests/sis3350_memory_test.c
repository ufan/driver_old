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




/*
int soll_data ;
int max_lword_cnt ;
int loop_cnt ;
*/


/****************************************************************************/
int main(int argc, char* argv[])
{

#define MAX_NUMBER_LWORDS 0x1000000  

int p;
int no_of_lwords ;
int i;

u_int32_t mod_base;
u_int32_t memory_addr;
u_int32_t *vmeData;
u_int32_t *vmeDataVerify;

/*u_int32_t blt_data[MAX_NUMBER_LWORDS] ;*/
u_int32_t get_lwords ;
int return_code, rc ;
int loop_counter ;

unsigned int loops = 0;
clock_t timeMeasureStart, timeMeasureStop;
double MBperSecWrite, MBperSecRead;
double MBperSecWriteAvg, MBperSecReadAvg;

struct timespec tp; 

struct timespec t1, t2;
double time_diff;


FILE *fpLog, *fpSpeed;


if (argc<4)
  {
  fprintf(stderr, "usage: %s PATH  SIS3350_VME_BASE_ADDR  NO_OF_LWORDS    \n", argv[0]);
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


if (no_of_lwords > MAX_NUMBER_LWORDS)
  {
     printf("no_of_lwords (0x%08x) must be lower then MAX_NUMBER_LWORDS (0x%08x)\n",no_of_lwords, MAX_NUMBER_LWORDS);
     return -1;
   }


fpLog = fopen("log.txt", "w");
if(fpLog == NULL){
	printf(" logfile fopen error\n");
	return 0;
}
 


#ifdef raus
clock_gettime(CLOCK_REALTIME, &t1);

clock_gettime(CLOCK_REALTIME, &t2);
 time_diff = (double)(t2.tv_sec - t1.tv_sec) + 1.e-9*(t2.tv_nsec - t1.tv_nsec);
#endif

fprintf(fpLog, "R/W Ramtest Logfile\nBLT32 Write - BLT32 Read\n\n");

vmeData = (u_int32_t*)malloc(no_of_lwords * sizeof(u_int32_t));
if(vmeData == NULL){
	printf(" malloc failed\n");
	return -1;
}

vmeDataVerify = (u_int32_t*)malloc(no_of_lwords * sizeof(u_int32_t));
if(vmeData == NULL){
	printf(" malloc failed\n");
	return -1;
}

	printf("R/W Ramtest\nBLT32 Write - BLT32 Read\n\n");

#define RANDOM_TEST
#define INCREMENT_TEST

return_code =  vme_A32D32_write(p, mod_base+0x01000000, 0x8000 ) ; /* enable test write mode */
if (return_code !=0) {
	printf("ERROR: vme_A32D32_write:   return_code = 0x%08x   addr = 0x%08x\n", return_code, mod_base+0x01000000 );         
	/*return -1 ; */
}

memory_addr = mod_base + 0x04000000 ;

while(1){

	MBperSecWriteAvg = 0.0;
	MBperSecReadAvg = 0.0;


#ifdef RANDOM_TEST

	/*random test */
	memset(vmeDataVerify, 0, sizeof(vmeDataVerify));
	for(i = 0;i < no_of_lwords;i++){
		vmeData[i] = (rand() << 16 | rand() ) & 0x0fff0fff; /* only 2 x 12bit*/;
	}

	timeMeasureStart = clock();
	rc = vme_A32BLT32_write(p, memory_addr, vmeData, no_of_lwords, &get_lwords);
	timeMeasureStop = clock();


	/*/printf("\r timeMeasureStart %.8f MB/s  timeMeasureStop: %.8f MB/s \n",(double)timeMeasureStart/CLOCKS_PER_SEC,(float)timeMeasureStop/CLOCKS_PER_SEC);*/
	/*printf("\r timeMeasureStart %.8f MB/s  timeMeasureStop: %.8f MB/s  %d \n",(double)timeMeasureStart, (double)timeMeasureStop, CLOCKS_PER_SEC);*/
	
	if(rc){
		printf("\n vme_A32BLT32_write failed: 0x%x\n", rc);
		break;
	}else{
		MBperSecWrite = (no_of_lwords * 4) / ((double)(timeMeasureStop - timeMeasureStart) / (double)CLOCKS_PER_SEC);
		MBperSecWrite /= 1000000.0;
		MBperSecWriteAvg += (double)MBperSecWrite;
	}


	timeMeasureStart = clock();
	rc = vme_A32BLT32_read(p, memory_addr, vmeDataVerify, no_of_lwords, &get_lwords);
	timeMeasureStop = clock();
	
	if(rc){
		printf("\n vme_A32BLT32_read failed: 0x%x\n", rc);
		break;
	}else{
		MBperSecRead = (no_of_lwords * 4) / ((double)(timeMeasureStop - timeMeasureStart) / (double)CLOCKS_PER_SEC);
		MBperSecRead /= 1000000.0;
		MBperSecReadAvg += (double)MBperSecRead;
	}

	rc = memcmp(vmeData, vmeDataVerify, sizeof(vmeData));
	if(rc){
		printf("\n\n memory verify failed (random)\n");
		for(i = 0;i < no_of_lwords;i++){
			if(i && ((i & 63) == 0))
				fprintf(fpLog, "--- page boundary ---(0x%08X)\n", i<<2);
			fprintf(fpLog, "0x%08X <-> 0x%08X", vmeData[i], vmeDataVerify[i]);
			if(vmeData[i] != vmeDataVerify[i]){
				fprintf(fpLog, " *\n");
			}else{
				fprintf(fpLog, "\n");
			}
		}
		break;
	}



#endif


#ifdef INCREMENT_TEST

	/* increment test */
	for(i = 0;i < no_of_lwords;i++){
		vmeData[i] = i & 0x0fff0fff; /* only 2 x 12bit*/
	}
	memset(vmeDataVerify, 0, sizeof(vmeDataVerify));
	
	timeMeasureStart = clock();
	rc = vme_A32BLT32_write(p, memory_addr, vmeData, no_of_lwords, &get_lwords);
	timeMeasureStop = clock();

	if(rc){
		printf("\n vme_A32BLT32_write failed: 0x%x\n", rc);
		break;
	}else{
		MBperSecWrite = (no_of_lwords * 4) / ((double)(timeMeasureStop - timeMeasureStart) / (double)CLOCKS_PER_SEC);
		MBperSecWrite /= 1000000.0;
		MBperSecWriteAvg += (double)MBperSecWrite;
	}


	timeMeasureStart = clock();
	rc = vme_A32BLT32_read(p, memory_addr, vmeDataVerify, no_of_lwords, &get_lwords);
	timeMeasureStop = clock();
	
	if(rc){
		printf("\n vme_A32BLT32_read failed: 0x%x\n", rc);
		break;
	}else{
		MBperSecRead = (no_of_lwords * 4) / ((double)(timeMeasureStop - timeMeasureStart) / (double)CLOCKS_PER_SEC);
		MBperSecRead /= 1000000.0;
		MBperSecReadAvg += (double)MBperSecRead;
	}

	rc = memcmp(vmeData, vmeDataVerify, sizeof(vmeData));
	if(rc){
		printf("\n\n memory verify failed (random)\n");
		for(i = 0;i < no_of_lwords;i++){
			if(i && ((i & 63) == 0))
				fprintf(fpLog, "--- page boundary ---(0x%08X)\n", i<<2);
			fprintf(fpLog, "0x%08X <-> 0x%08X", vmeData[i], vmeDataVerify[i]);
			if(vmeData[i] != vmeDataVerify[i]){
				fprintf(fpLog, " *\n");
			}else{
				fprintf(fpLog, "\n");
			}
		}
		break;
	}


#endif



	loops++;
	printf("\r successful loops: %d  write avg: %.3f MB/s  read avg: %.3f MB/s \n", loops, (double)MBperSecWriteAvg / 2.0, (double)MBperSecReadAvg / 2.0);
	/* if(kbhit()) 	break; */

}



free(vmeData);
free(vmeDataVerify);
fclose(fpLog);

close(p);

	fpSpeed = fopen("speed.txt", "w");
	if(fpSpeed == NULL){
		return 0;
	}
	fprintf(fpSpeed, "3104 VME Performance\n\n");
	fprintf(fpSpeed, "A32-BLT32 Write / Read - Blocksize: %d Bytes\n", no_of_lwords << 2);
	fprintf(fpSpeed, "\tWrite avg: %.3f MB/s\n\tRead avg: %.3f MB/s\n", (double)MBperSecWriteAvg / 2.0, (double)MBperSecReadAvg / 2.0);
	fprintf(fpSpeed, "\n\nEOF");

	fclose(fpSpeed);

return 0;
}

































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
int mod_base;
int no_of_lwords ;
int i;
int i_write_mode, i_read_mode;
int from_write_mode, from_read_mode;
int to_write_mode, to_read_mode;

u_int32_t *vmeData;
u_int32_t *vmeDataVerify;

/*u_int32_t blt_data[MAX_NUMBER_LWORDS] ;*/
u_int32_t get_lwords ;
int return_code, rc ;
int loop_counter ;
int error_counter ;

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

fprintf(fpLog, "R/W Ramtest Logfile\n Write -  Read\n\n");

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

	printf("R/W Ramtest\n Write -  Read\n\n");

#define RANDOM_TEST
#define INCREMENT_TEST


/* write_mode,  read_mode = 0  --> D32 */
/* write_mode,  read_mode = 1  --> BLT32 */
/* write_mode,  read_mode = 2  --> MBLT64 */

from_write_mode = 0;
to_write_mode   = 2;

from_read_mode = 0;
to_read_mode   = 2;

error_counter = 0;

while(1){

   for (i_write_mode = 0; i_write_mode<to_write_mode+1;i_write_mode++) {	
      for (i_read_mode = 0; i_read_mode<to_read_mode+1;i_read_mode++) {	
	MBperSecWriteAvg = 0.0;
	MBperSecReadAvg = 0.0;

	printf("\nMemory Test: Write Mode = %d   Read Mode = %d  ......", i_write_mode, i_read_mode );

#ifdef RANDOM_TEST

	/*random test */
	memset(vmeDataVerify, 0, sizeof(vmeDataVerify));
	for(i = 0;i < no_of_lwords;i++){
		vmeData[i] = rand() << 16 | rand();
	}

	timeMeasureStart = clock();

	if (i_write_mode == 0) {
		for (i=0;i<no_of_lwords;i++) {
			rc = vme_A32D32_write(p, mod_base+(4*i), vmeData[i]);
			//printf("\n vme_A32D32_write: 0x%x\n", rc);
			if(rc){
				printf("\n vme_A32D32_write failed: 0x%x\n", rc);
				break;
			}
		}
		get_lwords = i ;
	}

	if (i_write_mode == 1) {
		rc = vme_A32BLT32_write(p, mod_base, vmeData, no_of_lwords, &get_lwords);
		if(rc){
			printf("\n vme_A32BLT32_write failed: 0x%x\n", rc);
			break;
		}
	}
	if (i_write_mode == 2) {
		rc = vme_A32MBLT64_write(p, mod_base, vmeData, no_of_lwords, &get_lwords);
		if(rc){
			printf("\n vme_A32MBLT64_write failed: 0x%x\n", rc);
			break;
		}
	}

	
	if(rc){
		printf("\n vme_write failed: 0x%x\n", rc);
		break;
	}else{
		MBperSecWrite = (no_of_lwords * 4) / ((double)(timeMeasureStop - timeMeasureStart) / (double)CLOCKS_PER_SEC);
		MBperSecWrite /= 1000000.0;
		MBperSecWriteAvg += (double)MBperSecWrite;
	}


	timeMeasureStart = clock();
	if (i_read_mode == 0) {
		for (i=0;i<no_of_lwords;i++) {
			rc = vme_A32D32_read(p, mod_base+(4*i), &vmeDataVerify[i]);
			//if (i<4) printf("\n vme_A32D32_read: data  0x%08X\n", vmeDataVerify[i]);	
			//printf("\n vme_A32D32_read: 0x%x\n", rc);
			if(rc){
				printf("\n vme_A32D32_read failed: 0x%x\n", rc);
				break;
			}
		}
		get_lwords = i ;
	}

	if (i_read_mode == 1) {
		rc = vme_A32BLT32_read(p, mod_base, vmeDataVerify, no_of_lwords, &get_lwords);
		if(rc){
			printf("\n vme_A32BLT32_read failed: 0x%x\n", rc);
			break;
		}
	}
	if (i_read_mode == 2) {
		rc = vme_A32MBLT64_read(p, mod_base, vmeDataVerify, no_of_lwords, &get_lwords);
		if(rc){
			printf("\n vme_A32MBLT64_read failed: 0x%x\n", rc);
			break;
		}
	}
	timeMeasureStop = clock();
	
	if(rc){
		printf("\n vme_read failed: 0x%x\n", rc);
		break;
	}else{
		MBperSecRead = (no_of_lwords * 4) / ((double)(timeMeasureStop - timeMeasureStart) / (double)CLOCKS_PER_SEC);
		MBperSecRead /= 1000000.0;
		MBperSecReadAvg += (double)MBperSecRead;
	}

	rc = memcmp(vmeData, vmeDataVerify, sizeof(vmeData));
	if(rc){
		error_counter++;
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
		vmeData[i] = i;
	}
	memset(vmeDataVerify, 0, sizeof(vmeDataVerify));
	
	timeMeasureStart = clock();
	if (i_write_mode == 0) {
		for (i=0;i<no_of_lwords;i++) {
			rc = vme_A32D32_write(p, mod_base+(4*i), vmeData[i]);
			//printf("\n vme_A32D32_write: 0x%x\n", rc);
			if(rc){
				printf("\n vme_A32D32_write failed: 0x%x\n", rc);
				break;
			}
		}
		get_lwords = i ;
	}

	if (i_write_mode == 1) {
		rc = vme_A32BLT32_write(p, mod_base, vmeData, no_of_lwords, &get_lwords);
		if(rc){
			printf("\n vme_A32BLT32_write failed: 0x%x\n", rc);
			break;
		}
	}
	if (i_write_mode == 2) {
		rc = vme_A32MBLT64_write(p, mod_base, vmeData, no_of_lwords, &get_lwords);
		if(rc){
			printf("\n vme_A32MBLT64_write failed: 0x%x\n", rc);
			break;
		}
	}

	timeMeasureStop = clock();
	if(rc){
		printf("\n vme__write failed: 0x%x\n", rc);
		break;
	}else{
		MBperSecWrite = (no_of_lwords * 4) / ((double)(timeMeasureStop - timeMeasureStart) / (double)CLOCKS_PER_SEC);
		MBperSecWrite /= 1000000.0;
		MBperSecWriteAvg += (double)MBperSecWrite;
	}


	timeMeasureStart = clock();

	if (i_read_mode == 0) {
		for (i=0;i<no_of_lwords;i++) {
			rc = vme_A32D32_read(p, mod_base+(4*i), &vmeDataVerify[i]);
			//if (i<4) printf("\n vme_A32D32_read: data  0x%08X\n", vmeDataVerify[i]);	
			//printf("\n vme_A32D32_read: 0x%x\n", rc);
			if(rc){
				printf("\n vme_A32D32_read failed: 0x%x\n", rc);
				break;
			}
		}
		get_lwords = i ;
	}

	if (i_read_mode == 1) {
		rc = vme_A32BLT32_read(p, mod_base, vmeDataVerify, no_of_lwords, &get_lwords);
		if(rc){
			printf("\n vme_A32BLT32_read failed: 0x%x\n", rc);
			break;
		}
	}
	if (i_read_mode == 2) {
		rc = vme_A32MBLT64_read(p, mod_base, vmeDataVerify, no_of_lwords, &get_lwords);
		if(rc){
			printf("\n vme_A32MBLT64_read failed: 0x%x\n", rc);
			break;
		}
	}

	
	if(rc){
		printf("\n vme__read failed: 0x%x\n", rc);
		break;
	}else{
		MBperSecRead = (no_of_lwords * 4) / ((double)(timeMeasureStop - timeMeasureStart) / (double)CLOCKS_PER_SEC);
		MBperSecRead /= 1000000.0;
		MBperSecReadAvg += (double)MBperSecRead;
	}

	rc = memcmp(vmeData, vmeDataVerify, sizeof(vmeData));
	if(rc){
		error_counter++;
		printf("\n\n memory verify failed (increment)\n");
		for(i = 0;i < no_of_lwords;i++){
			if(i && ((i & 63) == 0)) fprintf(fpLog, "--- page boundary ---(0x%08X)\n", i<<2);
			fprintf(fpLog, "0x%08X <-> 0x%08X   i=0x%08X   Write Mode = %d   Read Mode = %d", vmeData[i], vmeDataVerify[i], i, i_write_mode, i_read_mode);
			if(vmeData[i] != vmeDataVerify[i]){
				fprintf(fpLog, " *\n");
			}else{
				fprintf(fpLog, "\n");
			}
		}
		break;
	}


#endif


     }
   }
   loops++;
   //printf("\r successful loops: %d  write avg: %.3f MB/s  read avg: %.3f MB/s \n", loops, (double)MBperSecWriteAvg / 2.0, (double)MBperSecReadAvg / 2.0);
   printf("\n\nloops: %d  error_counter %d \n", loops, error_counter);
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

































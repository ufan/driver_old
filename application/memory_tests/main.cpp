/*

  VME Ramtest for 3104

  */
#define __USE_POSIX199309

#ifdef __cplusplus
extern "C"{
#endif

#define BUFSIZE 0x800000 // 32MB
//#define BUFSIZE 0x400000 // 16MB
//#define BUFSIZE 0x200000 // 8MB
//#define BUFSIZE 0x100000 // 4MB
//#define BUFSIZE 0x80000 // 2MB
//#define BUFSIZE 0x20000 // 512KB
//#define BUFSIZE 0x100 // 512B

#define ONES_TEST
#define INCREMENT_TEST
#define PATTERN_TEST
#define ZEROES_TEST

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include "sis1100drv.h"
#include "sis310x_vme_calls.h"

int main(void);

int main(){

	U32 *vmeData;
	U32 *vmeDataVerify;
	U32 foo;
	unsigned char sis1100CardNum;
	int rc;
	unsigned int i;
	unsigned int loops = 0;
	//struct SIS1100_Device_Struct sis1100Handle;
	SIS1100_HANDLE sis1100Handle;
	FILE *fpLog, *fpSpeed;
	clock_t timeMeasureStart, timeMeasureStop;
	double MBperSecWrite, MBperSecRead;
	double MBperSecWriteAvg, MBperSecReadAvg;

	vmeData = (U32*)malloc(BUFSIZE * sizeof(U32));
	if(vmeData == NULL){
		printf(" malloc failed\n");
		return 0;
	}

	vmeDataVerify = (U32*)malloc(BUFSIZE * sizeof(U32));
	if(vmeData == NULL){
		printf(" malloc failed\n");
		return 0;
	}

	printf("R/W Ramtest\nBLT32 Write - BLT32 Read\n\n");

	rc = sis1100FindCards(SIS1100_ECMC, &sis1100CardNum);
	if(rc){
		printf(" sis1100FindCards failed: 0x%x\n", rc);
	}

	rc = sis1100Open(SIS1100_ECMC, sis1100CardNum - 1, &sis1100Handle);
	if(rc){
		printf(" sis1100Open failed: 0x%x\n", rc);
	}

	rc = sis1100Init(&sis1100Handle);
	if(rc){
		printf(" sis1100Init failed: 0x%x\n", rc);
	}

	fpLog = fopen("log.txt", "w");
	if(fpLog == NULL){
		printf(" logfile fopen error\n");
		return 0;
	}

	fprintf(fpLog, "R/W Ramtest Logfile\nBLT32 Write - BLT32 Read\n\n");

	printf(".");
	Sleep(1000);
	printf(".");
	Sleep(1000);
	printf(".\n\n");
	Sleep(1000);
	
	while(1){

		MBperSecWriteAvg = 0.0;
		MBperSecReadAvg = 0.0;

#ifdef PATTERN_TEST

		// random test
		memset(vmeDataVerify, 0, sizeof(vmeDataVerify));
		for(i = 0;i < BUFSIZE;i++){
			vmeData[i] = rand() << 16 | rand();
		}

		timeMeasureStart = clock();
		rc = vme_A32BLT32_write(&sis1100Handle, 0, vmeData, BUFSIZE, &foo);
		timeMeasureStop = clock();
		
		if(rc){
			printf("\n vme_A32BLT32_write failed: 0x%x\n", rc);
			break;
		}else{
			MBperSecWrite = (BUFSIZE * 4) / ((double)(timeMeasureStop - timeMeasureStart) / CLOCKS_PER_SEC);
			MBperSecWrite /= 1000000;
			MBperSecWriteAvg += (double)MBperSecWrite;
		}

		timeMeasureStart = clock();
		rc = vme_A32BLT32_read(&sis1100Handle, 0, vmeDataVerify, BUFSIZE, &foo);
		timeMeasureStop = clock();
		
		if(rc){
			printf("\n vme_A32BLT32_read failed: 0x%x\n", rc);
			break;
		}else{
			MBperSecRead = (BUFSIZE * 4) / ((double)(timeMeasureStop - timeMeasureStart) / CLOCKS_PER_SEC);
			MBperSecRead /= 1000000;
			MBperSecReadAvg += (double)MBperSecRead;
		}

		rc = memcmp(vmeData, vmeDataVerify, sizeof(vmeData));
		if(rc){
			printf("\n\n memory verify failed (random)\n");
			for(i = 0;i < BUFSIZE;i++){
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
#ifdef PATTERN_TEST

		// pattern test
		for(i = 0;i < BUFSIZE;i += 16){
			vmeData[i +  0] = 0xAA55AA55;
			vmeData[i +  1] = 0x55AA55AA;
			vmeData[i +  2] = 0x00112233;
			vmeData[i +  3] = 0x44556677;
			vmeData[i +  4] = 0x01010101;
			vmeData[i +  5] = 0x10101010;
			vmeData[i +  6] = 0xFFFFFFFF;
			vmeData[i +  7] = 0x00000000;
			vmeData[i +  8] = 0xF0F0F0F0;
			vmeData[i +  9] = 0xDEADBEEF;
			vmeData[i + 10] = 0xBAADF00D;
			vmeData[i + 11] = 0x0F0F0F0F;
			vmeData[i + 12] = 0x12345678;
			vmeData[i + 13] = 0x87654321;
			vmeData[i + 14] = 0x8899AABB;
			vmeData[i + 15] = 0xCCDDEEFF;
		}
		memset(vmeDataVerify, 0x0, sizeof(vmeDataVerify));

		timeMeasureStart = clock();
		rc = vme_A32BLT32_write(&sis1100Handle, 0, vmeData, BUFSIZE, &foo);
		timeMeasureStop = clock();
		
		if(rc){
			printf("\n vme_A32BLT32_write failed: 0x%x\n", rc);
			break;
		}else{
			MBperSecWrite = (BUFSIZE * 4) / ((double)(timeMeasureStop - timeMeasureStart) / CLOCKS_PER_SEC);
			MBperSecWrite /= 1000000;
			MBperSecWriteAvg += (double)MBperSecWrite;
		}
		timeMeasureStart = clock();
		rc = vme_A32BLT32_read(&sis1100Handle, 0, vmeDataVerify, BUFSIZE, &foo);
		timeMeasureStop = clock();
		
		if(rc){
			printf("\n vme_A32BLT32_read failed: 0x%x\n", rc);
			break;
		}else{
			MBperSecRead = (BUFSIZE * 4) / ((double)(timeMeasureStop - timeMeasureStart) / CLOCKS_PER_SEC);
			MBperSecRead /= 1000000;
			MBperSecReadAvg += (double)MBperSecRead;
		}

		rc = memcmp(vmeData, vmeDataVerify, sizeof(vmeData));
		if(rc){
			printf("\n\n memory verify failed (pattern)\n");
			for(i = 0;i < BUFSIZE;i++){
				if(i && ((i & 63) == 0))
					fprintf(fpLog, "--- page boundary (0x%08X)---\n", i<<2);
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

		// increment test
		for(i = 0;i < BUFSIZE;i++){
			vmeData[i] = i;
		}
		memset(vmeDataVerify, 0, sizeof(vmeDataVerify));

		timeMeasureStart = clock();
		rc = vme_A32BLT32_write(&sis1100Handle, 0, vmeData, BUFSIZE, &foo);
		timeMeasureStop = clock();
		
		if(rc){
			printf("\n vme_A32BLT32_write failed: 0x%x\n", rc);
			break;
		}else{
			MBperSecWrite = (BUFSIZE * 4) / ((double)(timeMeasureStop - timeMeasureStart) / CLOCKS_PER_SEC);
			MBperSecWrite /= 1000000;
			MBperSecWriteAvg += (double)MBperSecWrite;
		}
		timeMeasureStart = clock();
		rc = vme_A32BLT32_read(&sis1100Handle, 0, vmeDataVerify, BUFSIZE, &foo);
		timeMeasureStop = clock();
		
		if(rc){
			printf("\n vme_A32BLT32_read failed: 0x%x\n", rc);
			break;
		}else{
			MBperSecRead = (BUFSIZE * 4) / ((double)(timeMeasureStop - timeMeasureStart) / CLOCKS_PER_SEC);
			MBperSecRead /= 1000000;
			MBperSecReadAvg += (double)MBperSecRead;
		}

		rc = memcmp(vmeData, vmeDataVerify, sizeof(vmeData));
		if(rc){
			printf("\n\n memory verify failed (increment)\n");
			for(i = 0;i < BUFSIZE;i++){
				if(i && ((i & 63) == 0))
					fprintf(fpLog, "--- page boundary (0x%08X)---\n", i<<2);
				fprintf(fpLog, "0x%08X: 0x%08X <-> 0x%08X", i<<2, vmeData[i], vmeDataVerify[i]);
				if(vmeData[i] != vmeDataVerify[i]){
					fprintf(fpLog, " *\n");
				}else{
					fprintf(fpLog, "\n");
				}
			}
			break;
		}

#endif
#ifdef ONES_TEST

		// walking ones test
		vmeData[0] = 1;
		for(i = 1;i < BUFSIZE;i++){
			if((vmeData[i-1]<<1) == 0){
				vmeData[i] = 1;
			}else{
				vmeData[i] = vmeData[i-1]<<1;
			}
		}
		memset(vmeDataVerify, 0, sizeof(vmeDataVerify));

		timeMeasureStart = clock();
		rc = vme_A32BLT32_write(&sis1100Handle, 0, vmeData, BUFSIZE, &foo);
		timeMeasureStop = clock();
		
		if(rc){
			printf("\n vme_A32BLT32_write failed: 0x%x\n", rc);
			break;
		}else{
			MBperSecWrite = (BUFSIZE * 4) / ((double)(timeMeasureStop - timeMeasureStart) / CLOCKS_PER_SEC);
			MBperSecWrite /= 1000000;
			MBperSecWriteAvg += (double)MBperSecWrite;
		}
		timeMeasureStart = clock();
		rc = vme_A32BLT32_read(&sis1100Handle, 0, vmeDataVerify, BUFSIZE, &foo);
		timeMeasureStop = clock();
		
		if(rc){
			printf("\n vme_A32BLT32_read failed: 0x%x\n", rc);
			break;
		}else{
			MBperSecRead = (BUFSIZE * 4) / ((double)(timeMeasureStop - timeMeasureStart) / CLOCKS_PER_SEC);
			MBperSecRead /= 1000000;
			MBperSecReadAvg += (double)MBperSecRead;
		}

		rc = memcmp(vmeData, vmeDataVerify, sizeof(vmeData));
		if(rc){
			printf("\n\n memory verify failed (walking ones)\n");
			for(i = 0;i < BUFSIZE;i++){
				if(i && ((i & 63) == 0))
					fprintf(fpLog, "--- page boundary (0x%08X)---\n", i<<2);
				fprintf(fpLog, "0x%08X: 0x%08X <-> 0x%08X", i<<2, vmeData[i], vmeDataVerify[i]);
				if(vmeData[i] != vmeDataVerify[i]){
					fprintf(fpLog, " *\n");
				}else{
					fprintf(fpLog, "\n");
				}
			}
			break;
		}

#endif
#ifdef ZEROES_TEST

		// walking zeros test
		vmeData[0] = 0xFFFFFFFD;
		for(i = 1;i < BUFSIZE;i++){
			if(((vmeData[i-1]<<1)+1) == 0xFFFFFFFF){
				vmeData[i] = 0xFFFFFFFE;
			}else{
				vmeData[i] = (vmeData[i-1]<<1)+1;
			}
		}
		memset(vmeDataVerify, 0, sizeof(vmeDataVerify));

		timeMeasureStart = clock();
		rc = vme_A32BLT32_write(&sis1100Handle, 0, vmeData, BUFSIZE, &foo);
		timeMeasureStop = clock();
		
		if(rc){
			printf("\n vme_A32BLT32_write failed: 0x%x\n", rc);
			break;
		}else{
			MBperSecWrite = (BUFSIZE * 4) / ((double)(timeMeasureStop - timeMeasureStart) / CLOCKS_PER_SEC);
			MBperSecWrite /= 1000000;
			MBperSecWriteAvg += (double)MBperSecWrite;
		}
		timeMeasureStart = clock();
		rc = vme_A32BLT32_read(&sis1100Handle, 0, vmeDataVerify, BUFSIZE, &foo);
		timeMeasureStop = clock();
		
		if(rc){
			printf("\n vme_A32BLT32_read failed: 0x%x\n", rc);
			break;
		}else{
			MBperSecRead = (BUFSIZE * 4) / ((double)(timeMeasureStop - timeMeasureStart) / CLOCKS_PER_SEC);
			MBperSecRead /= 1000000;
			MBperSecReadAvg += (double)MBperSecRead;
		}

		rc = memcmp(vmeData, vmeDataVerify, sizeof(vmeData));
		if(rc){
			printf("\n\n memory verify failed (walking zeros)\n");
			for(i = 0;i < BUFSIZE;i++){
				if(i && ((i & 63) == 0))
					fprintf(fpLog, "--- page boundary (0x%08X)---\n", i<<2);
				fprintf(fpLog, "0x%08X: 0x%08X <-> 0x%08X", i<<2, vmeData[i], vmeDataVerify[i]);
				if(vmeData[i] != vmeDataVerify[i]){
					fprintf(fpLog, " *\n");
				}else{
					fprintf(fpLog, "\n");
				}
			}
			break;
		}

#endif

		printf("\r successful loops: %d  write avg: %.3fMB/s  read avg: %.3fMB/s", ++loops, (double)MBperSecWriteAvg / 4, (double)MBperSecReadAvg / 4);
		if(kbhit())
			break;
	}

	free(vmeData);
	free(vmeDataVerify);
	sis1100Close(&sis1100Handle);
	fclose(fpLog);

	fpSpeed = fopen("speed.txt", "w");
	if(fpSpeed == NULL){
		return 0;
	}
	fprintf(fpSpeed, "3104 VME Performance\n\n");
	fprintf(fpSpeed, "A32-BLT32 Write / Read - Blocksize: %d Bytes\n", BUFSIZE << 2);
	fprintf(fpSpeed, "\tWrite avg: %.3f MB/s\n\tRead avg: %.3f MB/s\n", (double)MBperSecWriteAvg / 4, (double)MBperSecReadAvg / 4);
	fprintf(fpSpeed, "\n\nEOF");

	fclose(fpSpeed);

	return 0;
}

#ifdef __cplusplus
}
#endif
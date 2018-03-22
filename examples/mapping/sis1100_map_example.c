#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "../../dev/pci/sis1100_map.h"
#include "../../dev/pci/sis1100_var.h"

int main(void) {

int pv;              /* path for VME calls */
int pc;              /* control (errorhandling) */
int i;
u_int32_t val, error;
struct sis1100_ctrl_reg reg;
char*      vme_base; /* hier landen die VME-Adressen */
u_int32_t* ctl_base; /* das sind die Register der PCI-Karte */

/* open */
pv=open("/dev/sis1100_00remote", O_RDWR, 0);
pc=open("/dev/sis1100_00ctrl", O_RDWR, 0);
if ((pv<0) || (pc<0)) {
    printf("error on opening\n");
    return -1;
}


/* mmap ? MByte VME space (koennte auch weniger sein) */
vme_base=mmap(0, 0x100000, PROT_READ|PROT_WRITE, MAP_SHARED, pv, 0);

/* mmap 4 KByte control space (weniger als eine page geht nicht) */
ctl_base=mmap(0, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, pc, 0);
if (vme_base==MAP_FAILED) {  
    printf("mapping VME failed\n");
    return -1;
}

if (ctl_base==MAP_FAILED) {
    printf("mapping CTL failed\n");
    return -1;
}


/* descriptoren fuellen */
/*
Beispiel:
drei VME-Module
I    Addr  0x38200000 A32
II   Addr  0x38000000 A32
III  Addr  0x0        A16


(III und IV passen zusammen in einen 4-MByte-Block.)
(wenn ein Modul mehr als 4 MByte braucht, muss man halt mehrere
Deskriptoren nehmen)
*/
u_int32_t descr[12]={
    0xff010800, /* header */
    0x9,        /* A32 */
  /*      0x38000000,                             */
    0x30000000,   /*                              */
    0,
    0xff010800, /* header */
    0x9,        /* A32 */
    0x38200000,   /*                              */
    0,
    0xff010800, /* header */
    0x29,       /* A16 */
    0x0,       /*                          */
    0,
};



    reg.offset=0x400;
    for (i=0; i<12; i++) {
        reg.val=descr[i];
        ioctl(pc, SIS1100_CTRL_WRITE, &reg);
        reg.offset+=4;
    }


struct sis1100_reg* regs=(struct sis1100_reg*)ctl_base;

/* ein paar Hilfspointer */
char* start1=vme_base;
char* start2=vme_base+2*0x400000;
char* start3=vme_base+2*0x400000;

u_int32_t* base_I   = (u_int32_t *)start1;
u_int16_t* base_II  = (u_int16_t *)start2;
u_int32_t* base_III = (u_int32_t *)start3+0x4000000%0x400000;
u_int16_t* base_IVa = (u_int16_t *)start3+0x4200000%0x400000;
u_int32_t* base_IVb = (u_int32_t *)start3+0x4200000%0x400000;


#ifdef noerr
/* und ein paar Zugriffe (billig, ohne Fehlerbehandlung) */
*base_I=17;        /* A24D16 write auf das erste Register von 'I' */
val=base_I[7];     /* A24D16 read des 7. Registers von 'I' */
*(base_II+2)=37;   /* A32D16 write auf das 3. Register von 'II' */
*(base_III+2)=41;  /* A32D32 write auf das 3. Register von 'III' */
*(base_IVb+2)=43;  /* A32D32 write auf das 3. Register von 'IV' */
*(base_IVa+4)=43;  /* A32D16 write auf die untere Haelfte desselben Registers */
#endif

/* switch on and off SIS3820 user LED */

 for (i=0;i<10;i++) {
   *base_I=0x1;
   sleep(1);
   *base_I=0x10000;
   sleep(1);
   printf("Id. reg: %8.8x\n",*(base_I+1));
   sleep(1);
 }

/* mapped access in loop */
 for (i=0;i<0xFFFF;i+=2) {
     *(base_I+i)=i;
 }

/* not tested below, mki 14.4.04 */

/* und einer nochmal richtig mit Fehlerbehandlung */
/* val=base_III[7]; */ 
 
do {
    error=regs->prot_error;
} while (error==sis1100_e_dlock);

if (error==sis1100_le_dlock) {
    error=regs->tc_hdr;
    if ((error&0x300)==0x300) {
        error=(error>>24)&0xff;
        error|=0x200;
    } else {
        val=regs->tc_dal;
        error=0;
    }
}
if (error) {
    if (error==sis1100_le_to)
        regs->p_balance=0;
    printf("game over!");

}

 return 0;
}

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <dev/pci/sis1100_var.h>
#include <dev/pci/sis1100_map.h>

#define CONTROLPATH "/dev/sis1100_00ctrl"
#define VMEPATH "/dev/sis1100_00remote"

#define BOARDADDR 0x38000000

int main(int argc, char* argv[])
{
    int pv, pc;
    volatile u_int32_t *cmap_; /* control space (local and remote) */
    volatile struct sis1100_reg *cmap; /* control space (local) */
    /*volatile struct sis3100_reg *rmap;*/ /* control space (remote) */
    volatile u_int32_t *vmap;          /* VME space */
    u_int32_t mapsize, val;

    if ((pc=open(CONTROLPATH, O_RDWR, 0))<0) {
        printf("open %s: %s\n", CONTROLPATH, strerror(errno));
        return 1;
    }
    if ((pv=open(VMEPATH, O_RDWR, 0))<0) {
        printf("open %s: %s\n", VMEPATH, strerror(errno));
        return 1;
    }

    /* map control space */
    if (ioctl(pc, SIS1100_MAPSIZE, &mapsize)<0) {
        printf("MAPSIZE: %s\n", strerror(errno));
        return 1;
    }
    if (mapsize!=0x1000) {
        printf("mapsize of control space is 0x%x (not 0x1000)\n", mapsize);
        return 1;
    }
    cmap_=mmap(0, mapsize, PROT_READ|PROT_WRITE, MAP_SHARED, pc, 0);
    if (cmap_==MAP_FAILED) {
        printf("mmap control space: %s\n", strerror(errno));
        return 1;
    }
    printf("control space mapped at %p\n", cmap_);
    cmap=(volatile struct sis1100_reg*)cmap_;
    /*rmap=(volatile struct sis3100_reg*)(cmap_+0x200); */

    /* map VME space */
    if (ioctl(pv, SIS1100_MAPSIZE, &mapsize)<0) {
        printf("MAPSIZE: %s\n", strerror(errno));
        return 1;
    }
    printf("mapsize of VME space is 0x%x\n", mapsize);
    if (mapsize==0) {
        printf("no mmap for VME space available\n");
        return 1;
    }

#if 0
    /* we should reduce mapsize to the amount really needed */
    mapsize=...;
#endif

    vmap=mmap(0, mapsize, PROT_READ|PROT_WRITE, MAP_SHARED, pv, 0);
    if (vmap==MAP_FAILED) {
        printf("mmap VME space: %s\n", strerror(errno));
        return 1;
    }
    printf("VME space mapped at %p\n", vmap);

    /* prepare a descriptor */
    cmap->sp1_descr[0].hdr=0x03010800; /* header */
    cmap->sp1_descr[0].am=0x2F;         /* A32 non privileged data access */
    cmap->sp1_descr[0].adl=0x0; /*BOARDADDR;*/
    cmap->sp1_descr[0].adh=0;

    /* access VME (simple but not correct) */
    while( 1 ){
      val=vmap[0x80000 >> 2];
    };
    printf("[0]: 0x%08x\n", val);
    val=vmap[1];    
    printf("[1]: 0x%08x\n", val);
    
    vmap[0] = 0xDEADBEEF;
    vmap[1] = 0x12345678;
    
    val=vmap[0];
    printf("[0]: 0x%08x\n", val);
    val=vmap[1];    
    printf("[1]: 0x%08x\n", val);

    /* the correct version (with error handling) */
    {
        u_int32_t prot_error;

        cmap->p_balance=0;
        prot_error=cmap->prot_error;

        val=vmap[0];
        do {
            prot_error=cmap->prot_error;
        } while (prot_error==sis1100_e_dlock);
        if (prot_error==sis1100_le_dlock) {
            val=cmap->tc_dal;
            prot_error=cmap->tc_hdr&0xff|0x200;	/*  */
        }
        if (prot_error) {
            printf("Error during VME read: 0x%x\n", prot_error);
        } else {
            printf("[0]: 0x%08x\n", val);
        }
    }

    munmap((void*)vmap, mapsize);
    munmap((void*)cmap_, 0x1000);
    close(pv);
    close(pc);

    return 0;
}

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

#include "dev/pci/sis1100_var.h"

/****************************************************************************/
int main(int argc, char* argv[])
{
        int p;
        if (argc!=2) {
                fprintf(stderr, "usage: %s path\n", argv[0]);
                return 1;
        }

        if ((p=open(argv[1], O_RDWR, 0))<0) return 1;

        while (1) {
                struct sis1100_ctrl_reg reg;

                int ioreg;
                reg.offset=0x80;
                if (ioctl(p, SIS3100_CONTROL_READ, &reg)<0) {
                        perror("SIS3100_CONTROL_READ");
                        return -1;
                }
                ioreg=reg.val;
                if (reg.error) printf("ioreg.error=%x\n", reg.error);

                printf("ioreg=0x%08x\n", ioreg);

                sleep(1);
        }

        close(p);
        return 0;    
}
/****************************************************************************/
/****************************************************************************/

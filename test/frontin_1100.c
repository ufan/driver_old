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

        if ((p=open(argv[1], O_RDWR, 0))<0) {
                fprintf(stderr, "open \"%s\"\n", argv[1]);
                return 1;
        }

        while (1) {
                struct sis1100_ctrl_reg reg;
                int optreg;

                reg.offset=0xF0;
                if (ioctl(p, SIS1100_CONTROL_READ, &reg)<0) {
                        perror("SIS1100_CONTROL_READ");
                        return -1;
                }
                optreg=reg.val;
                if (reg.error) printf("optreg.error=%x\n", reg.error);
                printf("optreg=0x%08x\n", optreg);

                sleep(1);
        }

        close(p);
        return 0;    
}
/****************************************************************************/
/****************************************************************************/

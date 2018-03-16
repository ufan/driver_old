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
        int p, count=0;
        if (argc!=2) {
                fprintf(stderr, "usage: %s path\n", argv[0]);
                return 1;
        }

        if ((p=open(argv[1], O_RDWR, 0))<0) return 1;

        while (1) {
                struct sis1100_ctrl_reg reg;
                int optreg;

                reg.offset=0xF0;
                if (ioctl(p, SIS1100_CONTROL_READ, &reg)<0) {
                        perror("SIS1100_CONTROL_READ");
                        return -1;
                }
                if (reg.error) printf("read optreg error=%x\n", reg.error);

                optreg=reg.val;
                optreg=(optreg&~0xf0) | ((count&0xf)<<4);

                reg.val=optreg;
                if (ioctl(p, SIS1100_CONTROL_WRITE, &reg)<0) {
                        perror("SIS1100_CONTROL_WRITE");
                        return -1;
                }
                if (reg.error) printf("write optreg error=%x\n", reg.error);

                reg.offset=0xF0;
                if (ioctl(p, SIS1100_CONTROL_READ, &reg)<0) {
                        perror("SIS1100_CONTROL_READ");
                        return -1;
                }
                if (reg.error) printf("read optreg error=%x\n", reg.error);

                optreg=reg.val;
                printf("count=%x optreg=0x%08x\n", count, optreg);

                sleep(1);
                count++;
        }

        close(p);
        return 0;    
}
/****************************************************************************/
/****************************************************************************/

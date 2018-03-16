#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "dev/pci/sis1100_var.h"

#define swap_int(x)  ((((x)>>24)&0x000000ff) |\
                      (((x)>> 8)&0x0000ff00) |\
                      (((x)<< 8)&0x00ff0000) |\
                      (((x)<<24)&0xff000000))

#define swap_short(x) ((((x)>>8)&0x000000ff) |\
                       (((x)<<8)&0x0000ff00))

int p;

struct sis1100_pipelist listent={0x03010000, 0x09, 0x200fc, 0};

struct sis1100_pipelist* list;

static int pipeline_read(int p, struct sis1100_pipelist* list, int listlen,
    u_int32_t* data)
{
    struct sis1100_pipe pipe;

    pipe.num=listlen;
    pipe.list=list;
    pipe.data=data;

    if (ioctl(p, SIS1100_PIPE, &pipe)<0) {
	printf("ioctl(SIS1100_PIPE): %s\n", strerror(errno));
        return -1;
    }
    if (pipe.error) printf("error=0x%x\n", pipe.error);
    return 0;
}

volatile int stop=0;

static void hand(int sig)
{
printf("signal %d\n", sig);
stop=1;
}

int main(int argc, char* argv[])
{
    int num, loopcount, i, j, *data;
    int *comp, comp_valid, dot, debug;
    struct sigaction act;

    if (argc<5)
      {
      fprintf(stderr, "usage: %s path reqcount loopcount debug\n", argv[0]);
      return 1;
      }
    if ((p=open(argv[1], O_RDWR, 0))<0) {
        perror("open");
        return 1;
    }

    num=atoi(argv[2]);
    loopcount=atoi(argv[3]);
    debug=atoi(argv[4]);

    act.sa_handler=hand;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGINT, &act, 0);
    sigaction(SIGQUIT, &act, 0);

    printf("listlen=%d; loopcount=%d\n", num, loopcount);

    list=(struct sis1100_pipelist*)malloc(num*sizeof(struct sis1100_pipelist));
    data=(u_int32_t*)malloc(num*sizeof(u_int32_t));
    comp=(u_int32_t*)malloc(num*sizeof(u_int32_t));
    comp_valid=0;

    if (!data || !list || !comp) {
    	printf("malloc: %s\n", strerror(errno));
	return 1;
    }
    for (i=0; i<num; i++) list[i]=listent;
    if (!debug)
        for (i=0; i<num; i++) data[i]=0x12345678; /* just for test */
    dot=10000/num;
    for (j=0; j<loopcount; j++) {
    	if (stop || (pipeline_read(p, list, num, data)<0)) goto raus;
        if (!debug) {
            if (comp_valid) {
                for (i=0; i<num; i++) {
                    if (comp[i]!=data[i]) printf("[%d] %08x-->%08x\n",
                        i, comp[i], data[i]);
                }
            } else {
                for (i=0; i<num; i++) comp[i]=data[i];
                comp_valid=1;
            }
        }
        if ((j%dot)==0) {printf("."); fflush(stdout);}
    }
raus:
    printf("tranferred %d words\n", j*num);
    /*
    for (i=0; i<num; i++)
    	printf("[%2d] %x: %08x\n", i, list[i].addr, swap_int(data[i])&0xffff);
    */
    close(p);
    return 0;
}

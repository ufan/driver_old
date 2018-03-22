/*
 * $ZEL: bin2mcs.c,v 1.1 2003/08/30 21:23:16 wuestner Exp $
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

static int
write_line(FILE* out, u_int8_t* b, int l)
{
    int8_t ck=0;
    int i;

    for (i=0; i<l; i++) ck-=b[i];
    b[l++]=ck;
    fprintf(out, ":");
    for (i=0; i<l; i++) fprintf(out, "%02X", b[i]);
    fprintf(out, "\n");
    return 0;
}

static int
write_addr(FILE* out, u_int32_t addr)
{
    u_int8_t b[7];
    b[0]=2;
    b[1]=b[2]=0;
    b[3]=4;
    b[4]=(addr>>8);
    b[5]=addr&0xff;
    write_line(out, b, 6);
    return 0;
}

static int
write_end(FILE* out)
{
    u_int8_t b[5];
    b[0]=b[1]=b[2]=0;
    b[3]=1;
    write_line(out, b, 4);
    return 0;
}

static int
write_data(FILE* out, u_int32_t addr, u_int8_t* b, int l)
{
    b[0]=l;
    b[1]=(addr>>8);
    b[2]=addr&0xff;
    b[3]=0;
    write_line(out, b, l+4);
    return 0;
}

static int
bin2mcs(int in, FILE* out)
{
    u_int32_t addr=0;
    u_int8_t buf[21];
    int res;

    do {
        if (!(addr&0xffff)) {
            write_addr(out, addr>>16);
        }
        res=read(in, buf+4, 16);
        if (res>0) write_data(out, addr&0xffff, buf, res);
        addr+=res;
    } while (res>0);
    write_end(out);
    return 0;
}

int main(int argc, char* argv[])
{
    fprintf(stderr, "\n==== bin to MCS converter; V0.1 ====\n\n");

    bin2mcs(0, stdout);
    return 0;
}

/*
 * $ZEL: mcs2bin.c,v 1.1 2003/08/30 21:23:21 wuestner Exp $
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

struct mcs_record {
	u_int32_t bc;
	u_int32_t addr;
	u_int32_t type;
	u_int8_t data[0x20];
        u_int32_t checksum;
};

static int /* 0: ok; 1: EOF; -1: error */
read_record(FILE* in, struct mcs_record* record, int line)
{
    const char* errortext=0;
    char s[1024];
    int res, num, pos=0;
    int extaddr=0;
    u_int8_t ck;

    if (!fgets(s, 1024, in)) { /* end of file or error */
        if (ferror(in)) {
            fprintf(stderr, "error reading input file\n");
            return -1;
        }
        if (feof(in)) {
            fprintf(stderr, "end of input file\n");
            return 1;
        }
        fprintf(stderr, "haeh???\n");
        return -1;
    } 

    res=sscanf(s, ":%02x%04x%02x%n",
            &record->bc, &record->addr, &record->type, &num);
    if (res<3) {
        errortext="header";
        goto fehler;
    }
    pos+=num;
    ck=record->bc+record->type+record->addr+(record->addr>>8);

    switch (record->type) {
    case 0: { /* data */
        u_int32_t d;
        int i;

        if (record->bc>16) {
            errortext="bytecount too large";
            goto fehler;
        }
        for (i=0; i<record->bc; i++) {
            res=sscanf(s+pos, "%02x%n", &d, &num);
            if (res<1) {
                errortext="data";
                goto fehler;
            }
            pos+=num;
            record->data[i]=d;
            ck+=d;
        }
        res=sscanf(s+pos, "%02x", &record->checksum);
        if (res!=1) {errortext="checksum"; goto fehler;}
        }
        break;
    case 1: /* end */
        return 0;
    case 2: /* extended address */
        /* nobreak */
    case 4: /* "      " "     " ??? */
        extaddr=0;
        res=sscanf(s+pos, "%04x%n", &record->addr, &num);
        if (res<1) {
            errortext="extended address";
            goto fehler;
        }
        pos+=num;
        ck+=record->addr+(record->addr>>8);
        res=sscanf(s+pos, "%02x", &record->checksum);
        if (res!=1) {errortext="checksum"; goto fehler;}
        break;
    default:
        errortext="unknown record type";
        goto fehler;
    }
    ck+=record->checksum;
    if (ck) {
        errortext="invalid checksum";
        goto fehler;
    }
    return 0;

fehler:
    fprintf(stderr, "cannot convert line %d (%s):\n", line, errortext);
    fprintf(stderr, "%s\n", s);
    return -1;
}

static int
mcs2bin(FILE* in, int op)
{
    int size=0, res, line=0;
    struct mcs_record record;
    int extaddr=0, nextaddr=0, addr;

    while (1) {
        res=read_record(in, &record, line);
        if (res) {
            return res<0?-1:size;
        }

        switch (record.type) {
        case 0: /* data */
            addr=record.addr+extaddr;
            if (addr!=nextaddr) {
                fprintf(stderr, "addess jump 0x%x --> 0x%x\n", nextaddr, addr);
                return -1;
            }
            nextaddr+=record.bc;
            res=write(op, record.data, record.bc);
            if (res!=record.bc) {
                fprintf(stderr, "error writing %d bytes: res=%d errno=%s\n",
                        record.bc, res, strerror(errno));
                return -1;
            }
            size+=record.bc;
            break;
        case 1: /* end */
            return size;
            break;
        case 2: /* extended address */
            /* nobreak */
        case 4: /* "      " "     " ??? */
            extaddr=record.addr<<16;
            break;
        }
        line++;
    }
    /* und wie zum Teufel sollen wir hier hinkommen? */
    return 0;
}

static int
pad_file(int op, int size)
{
    int n0=0, n1=0;
    int nsize=size;
    while (nsize) {
        n0++;
        n1+=nsize&1;
        nsize>>=1;
    }
    size=(1<<(n0-(n1==1)))-size;
    if (size) {
        u_int8_t* b=malloc(size);
        memset(b, 0xff, size);
        write(op, b, size);
        free(b);
    }
    return 0;
}

int main(int argc, char* argv[])
{
    int size;

    fprintf(stderr, "\n==== MCS to bin converter; V0.1 ====\n\n");

    size=mcs2bin(stdin, 1);
    fprintf(stderr, "size=%d\n", size);
    if (size>0) pad_file(1, size);
    return 0;
}

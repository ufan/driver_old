/*
 * $ZEL: mcs2bin2.c,v 1.1 2003/08/30 21:23:22 wuestner Exp $
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
	u_int8_t data[0x21];
        u_int32_t checksum;
};

static int /* 0: ok; 1: EOF; -1: error */
read_line(FILE* in, u_int8_t* b, int* len, int line)
{
    char *errorformat;
    char *s, ss[1024];
    int i, n, ll, res;
    u_int8_t cs;

    if (!(s=fgets(ss, 1024, in))) { /* end of file or error */
        if (ferror(in)) {
            fprintf(stderr, "error reading input file (line %d): %s\n",
                line, strerror(errno));
            return -1;
        }
        if (feof(in)) {
            fprintf(stderr, "end of input file (line %d)\n", line);
            return 1;
        }
        fprintf(stderr, "haeh???\n");
        return -1;
    }

    while (isspace(s[0])) s++;
    ll=strlen(s);
    while (ll && isspace(s[ll-1])) ll--;
    s[ll]=0;
    if (!ll) {
        fprintf(stderr, "line %d is empty\n", line);
        return -1;
    }
    if (s[0]!=':') {
        errorformat="invalid format of line %d\n";
        goto fehler;
    }
    s++; ll--; n=0;
    while (ll) {
        u_int32_t d;
        int num;
        res=sscanf(s, "%02x%n", &d, &num);
        b[n]=d;
        if ((res<1)||(num!=2)) {
            errorformat="error scanning line %d\n";
            goto fehler;
        }
        ll-=num; s+=num; n++;
    }
    cs=0;
    for (i=0; i<n; i++) cs+=b[i];
    if (cs) {
        errorformat="line %d has invalid checksum\n";
        goto fehler;
    }
    *len=n-1;
    return 0;

fehler:
    fprintf(stderr, errorformat, line);
    fprintf(stderr, "%s\n", ss);
    return -1;
}

static int /* 0: ok; 1: EOF; -1: error */
read_record(FILE* in, struct mcs_record* record, int line)
{
    const char* errorformat=0;
    int res;

    if ((res=read_line(in, record->data, &record->bc, line))) return res;

    if (record->bc<4) {
        errorformat="line %d too short\n";
        goto fehler;
    }
    if (record->bc-4!=record->data[0]) {
        errorformat="wrong bytecount in line %d\n";
        goto fehler;
    }
    record->bc=record->data[0];
    record->type=record->data[3];
    switch (record->type) {
    case 0: { /* data */
        if (record->bc>16) {
            errorformat="bytecount too large (line %d)\n";
            goto fehler;
        }
        record->addr=(record->data[1]<<8)|record->data[2];
        }
        break;
    case 1: /* end */
        if (record->bc) {
            errorformat="wrong bytecount in line %d (for record type 1)\n";
            goto fehler;
        }
        return 0;
    case 2: /* extended address */
        /* nobreak */
    case 4: /* "      " "     " ??? */
        if (record->bc!=2) {
            errorformat="wrong bytecount in line %d (for record type 4)\n";
            goto fehler;
        }
        record->addr=(record->data[4]<<8)|record->data[5];
        break;
    default:
        errorformat="unknown record type in line %d\n";
        goto fehler;
    }
    return 0;

fehler:
    fprintf(stderr, errorformat, line);
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
                fprintf(stderr, "addess jump 0x%x --> 0x%x (line %d)\n",
                    nextaddr, addr, line);
                return -1;
            }
            nextaddr+=record.bc;
            res=write(op, record.data+4, record.bc);
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

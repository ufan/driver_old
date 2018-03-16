#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "dev/pci/sis1100_var.h"

#define swap_int(x)  ((((x)>>24)&0x000000ff) |\
                      (((x)>> 8)&0x0000ff00) |\
                      (((x)<< 8)&0x00ff0000) |\
                      (((x)<<24)&0xff000000))
#define swap_short(x) ((((x)>>8)&0x000000ff) |\
                       (((x)<<8)&0x0000ff00))

/****************************************************************************/
static int read_local(int p, u_int32_t offs, u_int32_t* data)
{
struct sis1100_ctrl_reg reg;

reg.offset=offs;
if (ioctl(p, SIS1100_CONTROL_READ, &reg)<0)
  {
  fprintf(stderr, "ioctl(SIS1100_CONTROL_READ, offs=0x%x): %s\n",
      offs, strerror(errno));
  return -1;
  }
*data=reg.val;
return 0;
}
/****************************************************************************/
static int write_local(int p, u_int32_t offs, u_int32_t data)
{
struct sis1100_ctrl_reg reg;

reg.offset=offs;
reg.val=data;
if (ioctl(p, SIS1100_CONTROL_WRITE, &reg)<0)
  {
  fprintf(stderr, "ioctl(SIS1100_CONTROL_WRITE, offs=0x%x): %s\n",
      offs, strerror(errno));
  return -1;
  }
return 0;
}
/****************************************************************************/
/* !!! hier ist alles D16 */
static int vme_read(int p, u_int32_t addr, u_int16_t* val)
{
u_int32_t head, be, error, _val;

if (addr&2) {
    be=0xc;
    addr&=~2;
} else
    be=0x3;


head=(be<<24)|0x010802; /* remote space 1, am, start with address */
if (write_local(p, 0x80, head)<0) return -1; /* t_hdr */
if (write_local(p, 0x84, 9)<0) return -1;    /* t_am  */
if (write_local(p, 0x88, addr)<0) return -1; /* t_adl */

do {
  if(read_local(p, 0xac, &error)<0) return -1; /* prot_error */
} while (error==0x005); /* deadlock */

if (error) {
    printf("vme_read 0x%08x: err=0x%x\n", addr, error);
    return -1;
}
if (read_local(p, 0xa0, &_val)<0) return -1; /* tc_dal */
*val=swap_int(_val);
return 0;
}
/****************************************************************************/
static int vme_write(int p, u_int32_t addr, u_int16_t val)
{
u_int32_t head, be, error, _val;

_val=swap_int(val);
if (addr&2) {
    be=0xc;
    addr&=~2;
} else
    be=0x3;
head=(be<<24)|0x010c02; /* remote space 1, am, write, start with address */
if (write_local(p, 0x80, head)<0) return -1; /* t_hdr */
if (write_local(p, 0x84, 9)<0) return -1;    /* t_am  */
if (write_local(p, 0x90, _val)<0) return -1;  /* t_dal */
if (write_local(p, 0x88, addr)<0) return -1; /* t_adl */

do {
  error=read_local(p, 0xac, &error); /* prot_error */
} while (error==0x005); /* deadlock */

if (error) {
    printf("vme_write 0x%08x: err=0x%x\n", addr, error);
    return -1;
}
return 0;
}
/****************************************************************************/
int main(int argc, char* argv[])
{
int p;
int i;
u_int32_t base;
u_int16_t data;

if (argc<2)
  {
  fprintf(stderr, "usage: %s path\n", argv[0]);
  return 1;
  }
if ((p=open(argv[1], O_RDWR, 0))<0) return 1;

printf("search for CAEN-Modules:\n");
for (i=0, base=0; i<10; i++, base+=0x10000) {
    if (vme_read(p, base+0xfa, &data)==0) { /* kein Fehler */
    	if (data==0xfaf5) { /* CAEN-Module */
	    u_int16_t type, serial;
	    if (vme_read(p, base+0xfc, &type)<0) return -1;
	    if (vme_read(p, base+0xfe, &serial)<0) return -1;
	    printf("at 0x%x: type=0x%x; serial=%d\n", base, type, serial&0xfff);
	}
    }
}


close(p);
return 0;
}

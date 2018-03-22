#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>



#include "dev/pci/sis1100_var.h"
#include "sis3100_vme_calls.h"


/****************************************************************************/

/****************************************************************************/
int main(int argc, char* argv[])
{

int p;

int offset ;
u_int32_t data ;
int return_code ;



if (argc<2)
  {
  fprintf(stderr, "usage: %s path\n", argv[0]);
  return 1;
  }


if ((p=open(argv[1], O_RDWR, 0))<0) {
	perror("open");
	return 1;
}



  printf("set VME_RESET \n");
  offset = 0x00000100;
  data   = 0x00000002;
  return_code =  s3100_control_write(p, offset, data) ; 
  printf("s3100_control_write:   return_code = 0x%08x\n", return_code );         

  return_code =  s3100_control_read(p, offset, &data) ; 
  printf("s3100_control_read:   return_code = 0x%08x\n", return_code );         
  printf("s3100_control_read:   data        = 0x%08x\n", data );         



  sleep(1);


  offset = 0x00000100;
  data   = 0x00020000;
  return_code =  s3100_control_write(p, offset, data) ; 
  printf("s3100_control_write:   return_code = 0x%08x\n", return_code );         

  return_code =  s3100_control_read(p, offset, &data) ; 
  printf("s3100_control_read:   return_code = 0x%08x\n", return_code );         
  printf("s3100_control_read:   data        = 0x%08x\n", data );         

  printf("clear VME_RESET \n");


return 0;
}






















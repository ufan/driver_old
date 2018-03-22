/*===========================================================================*/
/*                                                                           */
/* File:             sis5100_camac_calls.c                                   */
/*                                                                           */
/* OS:               LINUX (Kernel >= 2.4.18                                 */
/*                                                                           */
/* Description:                                                              */
/*                                                                           */
/* Version:          0.1                                                     */
/*                                                                           */
/*                                                                           */
/* Generated:        22.06.04                                                */
/*                                                                           */
/* Author:           MKI                                                     */
/*                                                                           */
/* Last Change:                       Installation                           */
/*---------------------------------------------------------------------------*/
/* SIS GmbH                                                                  */
/* Harksheider Str. 102A                                                     */
/* 22399 Hamburg                                                             */
/*                                                                           */
/* http://www.struck.de                                                      */
/*                                                                           */
/*===========================================================================*/

#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#include "dev/pci/sis1100_var.h"  /* pfad im Makefile angeben */


#include "sis5100_camac_calls.h"


/*******************/
/*                 */
/*    CAMAC_NAF    */
/*                 */
/*******************/


int camac_write(int p, u_int32_t N,u_int32_t A,u_int32_t F, u_int32_t camac_data)
{
struct sis1100_camac_req req;
int res;

req.N=N;
req.A=A;
req.F=F;
req.data=camac_data;
req.error=0;

res=ioctl(p, SIS5100_CNAF, &req);

if (res) {
  return res;
} 
else {
  return req.error;
}
}

int camac_read(int p, u_int32_t N,u_int32_t A,u_int32_t F, u_int32_t* camac_data)
{
struct sis1100_camac_req req;
int res;

req.N=N;
req.A=A;
req.F=F;
req.error=0;

res=ioctl(p, SIS5100_CNAF, &req);

if (res) {
  return res;
} 
else {
  *camac_data=req.data;
  /* printf("reqdata: %8.8x\n",req.data); */
  return req.error;
}
}




/***********************/
/*                     */
/*    s5100_control    */
/*                     */
/***********************/


int s5100_control_read(int p, int offset, u_int32_t* data)
{
struct sis1100_ctrl_reg reg;
int error ;
  reg.offset = offset;
  error = (ioctl(p, SIS1100_CTRL_READ, &reg)<0)  ;
  *data = reg.val;
  return error ;
}


int s5100_control_write(int p, int offset, u_int32_t data)
{
struct sis1100_ctrl_reg reg;
int error ;
  reg.offset = offset; 
  reg.val  = data; 
  error = (ioctl(p, SIS1100_CTRL_WRITE, &reg)<0)  ;
  return error ;
}









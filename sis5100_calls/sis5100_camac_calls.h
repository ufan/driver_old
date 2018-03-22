/*===========================================================================*/
/*                                                                           */
/* File:             sis5100_camac_calls.h                                   */
/*                                                                           */
/* OS:               LINUX (Kernel >= 2.4.4                                  */
/*                                                                           */
/* Description:                                                              */
/*                                                                           */
/* Version:          1.0                                                     */
/*                                                                           */
/*                                                                           */
/* Generated:        18.12.01                                                */
/* Modified:         09.07.04   MKI                                          */
/*                                                                           */
/* Author:           TH                                                      */
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



/*****************/
/*               */
/*    CAMAC      */
/*               */
/*****************/
int camac_write(int p, u_int32_t N,u_int32_t A,u_int32_t F, u_int32_t camac_data) ;
int camac_read(int p, u_int32_t N,u_int32_t A,u_int32_t F, u_int32_t* camac_data) ;



/***********************/
/*                     */
/*    s5100_control    */
/*                     */
/***********************/
int s5100_control_read(int p, int offset, u_int32_t* data) ;
int s5100_control_write(int p, int offset, u_int32_t data) ;
















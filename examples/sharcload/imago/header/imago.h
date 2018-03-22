/*===========================================================================*/
/*                                                                           */
/* File:             imago.h                                                 */
/*                                                                           */
/* OS:               general                                                 */
/*                                                                           */
/* Description:      includes for imago.c                                    */
/*                                                                           */
/* Version:          1.0                                                     */
/*                                                                           */
/*                                                                           */
/* Generated:        13.02-02                                                */
/*                                                                           */
/* Author:           MKI (Dr. Matthias Kirsch)                               */
/*                                                                           */
/* Last Change:      xx.xx.02 MKI     Installation                           */
/*---------------------------------------------------------------------------*/
/* SIS GmbH                                                                  */
/* Harksheider Str. 102A                                                     */
/* 22399 Hamburg                                                             */
/*                                                                           */
/* http://www.struck.de                                                      */
/*                                                                           */
/*===========================================================================*/

#define CONFPATH "imago.conf"

#define SUCCESS      0
#define WORDWRITTEN  4

#define BEAMLEN 8192
#define BEAMBASE 0x1240000
#define ENERGYLEN 8192
#define ENERGYBASE 0x1248000

int sis3300_base;
int sis3600_base;
int sis3801_base;
int scaler_channels;
int period;
char* conf_path;
char dsppath[60];
char beampath[60];
char energypath[60];
































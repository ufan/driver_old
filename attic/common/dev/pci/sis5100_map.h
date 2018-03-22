/* $ZEL: sis5100_map.h,v 1.1 2003/01/09 12:09:20 wuestner Exp $ */

#ifndef _sis5100_map_h_
#define _sis5100_map_h_

struct sis5100_reg {
    u_int32_t ident;              /* 0x000 */
    u_int32_t optical_sr;         /* 0x004 */
    u_int32_t optical_cr;         /* 0x008 */
    u_int32_t dsp_sc;             /* 0x008 */
    u_int32_t dummy;
};

/* bits in in_out */
#define sis5100_io_flat_out1 (1<<0)
#define sis5100_io_flat_out2 (1<<1)
#define sis5100_io_flat_out3 (1<<2)
#define sis5100_io_flat_out4 (1<<3)
#define sis5100_io_lemo_out1 (1<<4)
#define sis5100_io_lemo_out2 (1<<5)
#define sis5100_io_lemo_out3 (1<<6)
/* clear is (io_*_out?)<<16 */
#define sis5100_io_flat_in1 (1<<16)
#define sis5100_io_flat_in2 (1<<17)
#define sis5100_io_flat_in3 (1<<18)
#define sis5100_io_flat_in4 (1<<19)
#define sis5100_io_lemo_in1 (1<<20)
#define sis5100_io_lemo_in2 (1<<21)
#define sis5100_io_lemo_in3 (1<<22)

/* bits in dsp_sc */
#define sis5100_dsp_run        (1<<8)
#define sis5100_dsp_boot_eprom (1<<9)
#define sis5100_dsp_boot_ctrl  (1<<11)

#define sis5100_dsp_available  (1<<24)
#define sis5100_dsp_flag0      (1<<28)
#define sis5100_dsp_flag1      (1<<29)
#define sis5100_dsp_flag2      (1<<30)
#define sis5100_dsp_flag3      (1<<31)

/* error codes */
#define sis5100_sis3100_re_berr        0x211 /* Bus Error */
#define sis5100_sis3100_re_retr        0x212 /* Retry */
#define sis5100_sis3100_re_atimeout    0x214 /* Arbitration timeout */

#endif

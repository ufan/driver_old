/* $ZEL: sis3100_map.h,v 1.3 2003/01/09 12:08:53 wuestner Exp $ */

#ifndef _sis3100_map_h_
#define _sis3100_map_h_

struct sis3100_reg {
    u_int32_t ident;              /* 0x000 */
    u_int32_t optical_sr;         /* 0x004 */
    u_int32_t optical_cr;         /* 0x008 */
    u_int32_t res0[29];
    u_int32_t in_out;             /* 0x080 */
    u_int32_t in_latch_irq;       /* 0x084 */
    u_int32_t res1[30];
    u_int32_t vme_master_sc;      /* 0x100 */
    u_int32_t vme_irq_sc;         /* 0x104 */
    u_int32_t res2[62];
    u_int32_t vme_slave_sc;       /* 0x200 */
    u_int32_t dma_write_counter;  /* 0x204 */
    u_int32_t res3[62];
    u_int32_t dsp_sc;             /* 0x300 */
    u_int32_t res4[63];
    u_int32_t vme_addr_map[256];  /* 0x400 */
};

/* bits in in_out */
#define sis3100_io_flat_out1 (1<<0)
#define sis3100_io_flat_out2 (1<<1)
#define sis3100_io_flat_out3 (1<<2)
#define sis3100_io_flat_out4 (1<<3)
#define sis3100_io_lemo_out1 (1<<4)
#define sis3100_io_lemo_out2 (1<<5)
#define sis3100_io_lemo_out3 (1<<6)
/* clear is (io_*_out?)<<16 */
#define sis3100_io_flat_in1 (1<<16)
#define sis3100_io_flat_in2 (1<<17)
#define sis3100_io_flat_in3 (1<<18)
#define sis3100_io_flat_in4 (1<<19)
#define sis3100_io_lemo_in1 (1<<20)
#define sis3100_io_lemo_in2 (1<<21)
#define sis3100_io_lemo_in3 (1<<22)

/* bits in vme_master_sc */
#define vme_system_controller_enable (1<<0)
#define vme_sys_reset         (1<<1)
#define vme_lemo_out_reset    (1<<2)
#define vme_power_on_reset    (1<<3)
#define vme_request_level     (3<<4)
#define vme_requester_type    (1<<6)
#define vme_user_led          (1<<7)
#define vme_enable_retry      (1<<8)
#define vme_long_timer        (3<<12)
#define vme_berr_timer        (3<<14)
#define vme_system_controller (1<<16)

/* bits in dsp_sc */
#define sis3100_dsp_run        (1<<8)
#define sis3100_dsp_boot_eprom (1<<9)
#define sis3100_dsp_boot_ctrl  (1<<11)

#define sis3100_dsp_available  (1<<24)
#define sis3100_dsp_flag0      (1<<28)
#define sis3100_dsp_flag1      (1<<29)
#define sis3100_dsp_flag2      (1<<30)
#define sis3100_dsp_flag3      (1<<31)

/* error codes */
#define sis3100_re_berr        0x211 /* Bus Error */
#define sis3100_re_retr        0x212 /* Retry */
#define sis3100_re_atimeout    0x214 /* Arbitration timeout */

#endif

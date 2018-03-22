/* $ZEL: sis1100_map.h,v 1.2 2003/01/09 12:06:29 wuestner Exp $ */

#ifndef _sis1100_map_h_
#define _sis1100_map_h_

struct sis1100_reg {
    u_int32_t ident;
    u_int32_t sr;
    u_int32_t cr;
    u_int32_t semaphore;
    u_int32_t doorbell;
    u_int32_t res0[3];
    u_int32_t mailbox[8];
    u_int32_t res1[16];
    u_int32_t t_hdr;
    u_int32_t t_am;
    u_int32_t t_adl;
    u_int32_t t_adh;
    u_int32_t t_dal;
    u_int32_t t_dah;
    u_int32_t res2;
    u_int32_t tc_hdr;  
    u_int32_t tc_dal; 
    u_int32_t tc_dah;
    u_int32_t p_balance;
    u_int32_t prot_error;
    u_int32_t d0_bc;
    u_int32_t d0_bc_buf;
    u_int32_t d0_bc_blen;
    u_int32_t d_hdr;
    u_int32_t d_am;
    u_int32_t d_adl;
    u_int32_t d_adh;
    u_int32_t d_bc;
    u_int32_t res4[2];
    u_int32_t rd_pipe_buf;
    u_int32_t rd_pipe_blen;
    u_int32_t res5[2];
    u_int32_t tp_special;
    u_int32_t tp_data;
    u_int32_t opt_csr;
    u_int32_t jtag_csr;
    u_int32_t res6[2];
    u_int32_t mailext[192];

    struct {
        u_int32_t hdr;
        u_int32_t am;
        u_int32_t adl;
        u_int32_t adh;
    } sp1_descr[64];
};

/* irq bits in sr and cr */
#define irq_synch_chg     (1<< 4)
#define irq_inh_chg       (1<< 5)
#define irq_sema_chg      (1<< 6)
#define irq_rec_violation (1<< 7)
#define irq_reset_req     (1<< 8)
#define irq_dma_eot       (1<< 9)
#define irq_mbx0          (1<<10)
#define irq_s_xoff        (1<<11)
#define irq_lemo_in_0_chg (1<<12)
#define irq_lemo_in_1_chg (1<<13)
#define irq_lemo_in_chg   (irq_lemo_in_0_chg|irq_lemo_in_1_chg)
#define irq_prot_end      (1<<14)
#define irq_prot_l_err    (1<<15)

#define sis1100_all_irq          0xfff0

/* bits in sr (without irqs) */
#define sr_rx_synch     (1<<0)
#define sr_tx_synch     (1<<1)
#define sr_synch        (sr_rx_synch|sr_tx_synch)
#define sr_inhibit      (1<<2)
#define sr_configured   (1<<3)
#define sr_dma0_blocked (1<<16)
#define sr_no_pread_buf (1<<17)
#define sr_prot_err     (1<<18)
#define sr_bus_tout     (1<<19)
#define sr_tp_special   (1<<20)
#define sr_tp_data      (1<<21)
#define sr_abort_dma    (1<<31)

/* bits in cr (without irqs) */
#define cr_reset        (1<<0)
#define cr_transparent  (1<<1)
#define cr_ready        (1<<2)
#define cr_bigendian    (1<<3)
#define cr_rem_reset    (1<<16)

/* bits in opt_csr  (without "internals") */
#define opt_lemo_out_0  (1<<4)
#define opt_lemo_out_1  (1<<5)
#define opt_led_0       (1<<6)
#define opt_led_1       (1<<7)
#define opt_lemo_in_0   (1<<8)
#define opt_lemo_in_1   (1<<9)

/* error codes */
#define sis1100_e_dlock     0x005
#define sis1100_le_synch    0x101
#define sis1100_le_nrdy     0x102
#define sis1100_le_xoff     0x103
#define sis1100_le_resource 0x104
#define sis1100_le_dlock    0x105
#define sis1100_le_to       0x107
#define sis1100_re_nrdy     0x202
#define sis1100_re_prot     0x206
#define sis1100_re_to       0x207
#define sis1100_re_berr     0x208
#define sis1100_re_ferr     0x209

#endif

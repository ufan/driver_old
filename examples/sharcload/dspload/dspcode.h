#ifndef _dspcode_h_
#define _dspcode_h_

struct section {
    char* s_name;
    u_int32_t s_flags;
    u_int32_t s_size;
    u_int32_t s_paddr;
    void* addr;
    int is_code;
};

struct code_info {
    int (*free)(struct code_info*);
    void *opaque_info;
    struct section* sections;
    int nsect;
};

size_t read_dsp_ldr(int p, const char* codepath, u_int8_t** code,
        struct code_info* info);
size_t read_dsp_coff(int p, const char* codepath, u_int8_t** code,
        struct code_info* info);
size_t read_dsp_raw(int p, const char* codepath, u_int8_t** code,
        struct code_info* info);

#endif

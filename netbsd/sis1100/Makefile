#

.PATH:	../conftabs
.PATH:	../../dev/pci

MKMAN=	no

KMOD=	sis1100

CPPFLAGS+= -DLKMNAME=$(KMOD) -DCDEVSW=$(KMOD)cdevsw -DCFDATA=$(KMOD)cfdata
CPPFLAGS+= -I../..
CFLAGS+= -Wall -Wmissing-prototypes -fstrict-prototypes

SRCS= \
    pcilkm_glue.c \
    plx9054_reset.c \
    plx9054dma_netbsd.c \
    sis1100_autoconf_netbsd.c \
    sis1100_init.c \
    sis1100_open.c \
    sis1100_irq.c \
    sis1100_irq_thread.c \
    sis1100_irq_handler.c \
    sis1100_mbx0_handler.c \
    sis3100rem_irq.c \
    sis5100rem_irq.c\
    zellvd_rem_irq.c \
    sis1100_init_remote.c \
    sis1100rem_init.c \
    sis3100rem_init.c \
    sis5100rem_init.c \
    zellvd_rem_init.c \
    sis1100_init_sdram.c \
    sis1100_ioctl.c \
    sis1100_mmap.c \
    sis1100_read.c \
    sis1100_write.c \
    sis1100_poll.c \
    sis1100_read_block.c \
    sis1100_write_block.c \
    sis1100_pipe_netbsd.c \
    sis1100_write_pipe.c \
    sis1100_tmp_read.c \
    sis1100_tmp_write.c \
    sis1100_read_dma.c \
    sis1100_read_dma_netbsd.c \
    sis1100_write_dma.c \
    sis1100_write_dma_netbsd.c \
    sis1100_read_loop.c \
    sis1100_write_loop.c \
    sis1100_front_io.c \
    sis1100rem_front_io.c \
    sis3100rem_front_io.c \
    sis5100rem_front_io.c \
    sis1100_synch_handler.c \
    sis1100_lemo_handler.c \
    sis1100_dsp.c \
    sis1100_eeprom.c \
    sis1100_dma_alloc_netbsd.c \
    sis1100_ddma_map.c \
    sis1100_ddma_map_netbsd.c \
    sis1100_ddma_startstop.c \
    pandapixel_rem_init.c \
    pandapixel_rem_irq.c

load:
	modload -A /var/run/kernel.syms -p makedevs sis1100.o

unload:
	modunload -n sis1100

.include <bsd.kmod.mk>

CC           := gcc
WFLAGS       := -Wstrict-prototypes -Wmissing-prototypes \
                -Wmissing-declarations -Wimplicit -Wreturn-type -Wunused \
                -Wcomment -Wformat
#                -Wuninitialized -Werror
DRIVER_PATH  := ../..
CPPFLAGS     := -I$(DRIVER_PATH) -I$(DRIVER_PATH)/sis3100_calls
 

CFLAGS       := -g -ansi $(WFLAGS) -L$(DRIVER_PATH)/sis3100_calls

srcdir       := .
cfiles       := $(wildcard $(srcdir)/*.c)
EXEC         := $(cfiles:.c=)

.PHONY: all
all: $(EXEC)

vme_read_irqack: vme_read_irqack.c $(DRIVER_PATH)/sis3100_calls/lib_sis3100.a
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100

vme_read_d8: vme_read_d8.c $(DRIVER_PATH)/sis3100_calls/lib_sis3100.a
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100

vme_read_d16: vme_read_d16.c $(DRIVER_PATH)/sis3100_calls/lib_sis3100.a
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100

vme_read_d32: vme_read_d32.c $(DRIVER_PATH)/sis3100_calls/lib_sis3100.a
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100

vme_read_crcsr_d32: vme_read_crcsr_d32.c $(DRIVER_PATH)/sis3100_calls/lib_sis3100.a
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100

vme_read_blt32: vme_read_blt32.c $(DRIVER_PATH)/sis3100_calls/lib_sis3100.a
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100

vme_read_2esst320: vme_read_2esst320.c $(DRIVER_PATH)/sis3100_calls/lib_sis3100.a
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100

vme_write_d8: vme_write_d8.c $(DRIVER_PATH)/sis3100_calls/lib_sis3100.a
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100

vme_write_d16: vme_write_d16.c $(DRIVER_PATH)/sis3100_calls/lib_sis3100.a
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100

vme_write_a24d16: vme_write_a24d16.c $(DRIVER_PATH)/sis3100_calls/lib_sis3100.a
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100

vme_write_d32: vme_write_d32.c $(DRIVER_PATH)/sis3100_calls/lib_sis3100.a
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100

vme_read_dma_a24d16: vme_read_dma_a24d16.c $(DRIVER_PATH)/sis3100_calls/lib_sis3100.a
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100

vme_write_dma_a24d16: vme_write_dma_a24d16.c $(DRIVER_PATH)/sis3100_calls/lib_sis3100.a
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100

vme_write_blt32: vme_write_blt32.c $(DRIVER_PATH)/sis3100_calls/lib_sis3100.a
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100

test: test.c $(DRIVER_PATH)/sis3100_calls/lib_sis3100.a
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100

vme_fill_d32: vme_fill_d32.c $(DRIVER_PATH)/sis3100_calls/lib_sis3100.a
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100


clean:
	rm -f *.o core $(EXEC)

depend:
	cp Makefile Makefile.bak
	sed -e '/^# DO NOT DELETE THIS LINE/,$$d' < Makefile.bak > Makefile
	echo '# DO NOT DELETE THIS LINE' >> Makefile
	echo ' ' >> Makefile
	for i in $(cfiles) ; do \
	  echo checking $$i ; \
	  $(CC) -M $(CPPFLAGS) $(srcdir)/$$i >> Makefile ; \
	done
# DO NOT DELETE THIS LINE
 
test.o: test.c /usr/include/stdio.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/errno.h /usr/include/bits/errno.h \
  /usr/include/linux/errno.h /usr/include/asm/errno.h \
  /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h \
  /usr/include/string.h /usr/include/xlocale.h /usr/include/sys/types.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h /usr/include/sys/ioctl.h \
  /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h \
  /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h \
  /usr/include/sys/ttydefaults.h ../../dev/pci/sis1100_var.h \
  /usr/include/linux/ioctl.h ../../sis3100_calls/sis3100_vme_calls.h
vme_fill_d32.o: vme_fill_d32.c /usr/include/stdio.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/errno.h /usr/include/bits/errno.h \
  /usr/include/linux/errno.h /usr/include/asm/errno.h \
  /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h \
  /usr/include/string.h /usr/include/xlocale.h /usr/include/sys/types.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h /usr/include/sys/ioctl.h \
  /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h \
  /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h \
  /usr/include/sys/ttydefaults.h ../../dev/pci/sis1100_var.h \
  /usr/include/linux/ioctl.h ../../sis3100_calls/sis3100_vme_calls.h
vme_read_blt32.o: vme_read_blt32.c /usr/include/stdio.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/errno.h /usr/include/bits/errno.h \
  /usr/include/linux/errno.h /usr/include/asm/errno.h \
  /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h \
  /usr/include/string.h /usr/include/xlocale.h /usr/include/sys/types.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h /usr/include/sys/ioctl.h \
  /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h \
  /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h \
  /usr/include/sys/ttydefaults.h ../../dev/pci/sis1100_var.h \
  /usr/include/linux/ioctl.h ../../sis3100_calls/sis3100_vme_calls.h
vme_read_d16.o: vme_read_d16.c /usr/include/stdio.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/errno.h /usr/include/bits/errno.h \
  /usr/include/linux/errno.h /usr/include/asm/errno.h \
  /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h \
  /usr/include/string.h /usr/include/xlocale.h /usr/include/sys/types.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h /usr/include/sys/ioctl.h \
  /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h \
  /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h \
  /usr/include/sys/ttydefaults.h ../../dev/pci/sis1100_var.h \
  /usr/include/linux/ioctl.h ../../sis3100_calls/sis3100_vme_calls.h
vme_read_d32.o: vme_read_d32.c /usr/include/stdio.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/errno.h /usr/include/bits/errno.h \
  /usr/include/linux/errno.h /usr/include/asm/errno.h \
  /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h \
  /usr/include/string.h /usr/include/xlocale.h /usr/include/sys/types.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h /usr/include/sys/ioctl.h \
  /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h \
  /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h \
  /usr/include/sys/ttydefaults.h ../../dev/pci/sis1100_var.h \
  /usr/include/linux/ioctl.h ../../sis3100_calls/sis3100_vme_calls.h
vme_read_d8.o: vme_read_d8.c /usr/include/stdio.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/errno.h /usr/include/bits/errno.h \
  /usr/include/linux/errno.h /usr/include/asm/errno.h \
  /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h \
  /usr/include/string.h /usr/include/xlocale.h /usr/include/sys/types.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h /usr/include/sys/ioctl.h \
  /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h \
  /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h \
  /usr/include/sys/ttydefaults.h ../../dev/pci/sis1100_var.h \
  /usr/include/linux/ioctl.h ../../sis3100_calls/sis3100_vme_calls.h
vme_read_irqack.o: vme_read_irqack.c /usr/include/stdio.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/errno.h /usr/include/bits/errno.h \
  /usr/include/linux/errno.h /usr/include/asm/errno.h \
  /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h \
  /usr/include/string.h /usr/include/xlocale.h /usr/include/sys/types.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h /usr/include/sys/ioctl.h \
  /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h \
  /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h \
  /usr/include/sys/ttydefaults.h ../../dev/pci/sis1100_var.h \
  /usr/include/linux/ioctl.h ../../sis3100_calls/sis3100_vme_calls.h
vme_write_blt32.o: vme_write_blt32.c /usr/include/stdio.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/errno.h /usr/include/bits/errno.h \
  /usr/include/linux/errno.h /usr/include/asm/errno.h \
  /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h \
  /usr/include/string.h /usr/include/xlocale.h /usr/include/sys/types.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h /usr/include/sys/ioctl.h \
  /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h \
  /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h \
  /usr/include/sys/ttydefaults.h ../../dev/pci/sis1100_var.h \
  /usr/include/linux/ioctl.h ../../sis3100_calls/sis3100_vme_calls.h
vme_write_d16.o: vme_write_d16.c /usr/include/stdio.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/errno.h /usr/include/bits/errno.h \
  /usr/include/linux/errno.h /usr/include/asm/errno.h \
  /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h \
  /usr/include/string.h /usr/include/xlocale.h /usr/include/sys/types.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h /usr/include/sys/ioctl.h \
  /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h \
  /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h \
  /usr/include/sys/ttydefaults.h ../../dev/pci/sis1100_var.h \
  /usr/include/linux/ioctl.h ../../sis3100_calls/sis3100_vme_calls.h
vme_write_d32.o: vme_write_d32.c /usr/include/stdio.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/errno.h /usr/include/bits/errno.h \
  /usr/include/linux/errno.h /usr/include/asm/errno.h \
  /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h \
  /usr/include/string.h /usr/include/xlocale.h /usr/include/sys/types.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h /usr/include/sys/ioctl.h \
  /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h \
  /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h \
  /usr/include/sys/ttydefaults.h ../../dev/pci/sis1100_var.h \
  /usr/include/linux/ioctl.h ../../sis3100_calls/sis3100_vme_calls.h
vme_write_d8.o: vme_write_d8.c /usr/include/stdio.h \
  /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stddef.h \
  /usr/include/bits/types.h /usr/include/bits/wordsize.h \
  /usr/include/bits/typesizes.h /usr/include/libio.h \
  /usr/include/_G_config.h /usr/include/wchar.h /usr/include/bits/wchar.h \
  /usr/include/gconv.h \
  /usr/lib/gcc-lib/i586-suse-linux/3.3.3/include/stdarg.h \
  /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h \
  /usr/include/errno.h /usr/include/bits/errno.h \
  /usr/include/linux/errno.h /usr/include/asm/errno.h \
  /usr/include/asm-generic/errno.h /usr/include/asm-generic/errno-base.h \
  /usr/include/string.h /usr/include/xlocale.h /usr/include/sys/types.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/bits/time.h \
  /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/environments.h \
  /usr/include/bits/confname.h /usr/include/getopt.h \
  /usr/include/stdlib.h /usr/include/bits/waitflags.h \
  /usr/include/bits/waitstatus.h /usr/include/alloca.h \
  /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/sys/stat.h \
  /usr/include/bits/stat.h /usr/include/sys/ioctl.h \
  /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h \
  /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h \
  /usr/include/sys/ttydefaults.h ../../dev/pci/sis1100_var.h \
  /usr/include/linux/ioctl.h ../../sis3100_calls/sis3100_vme_calls.h

 CC           := gcc
WFLAGS       := -Wstrict-prototypes -Wmissing-prototypes \
                -Wmissing-declarations -Wimplicit -Wreturn-type -Wunused \
                -Wcomment -Wformat
#                -Wuninitialized -Werror
#CPPFLAGS     := -I/usr/local/lkmx/sys
DRIVER_PATH  := ../../..
CPPFLAGS     := -I$(DRIVER_PATH) -I$(DRIVER_PATH)/sis3100_calls
CFLAGS       := -g -ansi $(WFLAGS) -L$(DRIVER_PATH)/sis3100_calls

srcdir       := .
cfiles       := $(wildcard $(srcdir)/*.c)
EXEC         := $(cfiles:.c=)

.PHONY: all
all: $(EXEC)

imago: imago.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< -l_sis3100

histo_read: histo_read.c
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
 
imago.o: ././imago.c /usr/include/stdio.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
 /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h \
 /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h \
 /usr/include/bits/sched.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/wchar.h \
 /usr/include/bits/wchar.h /usr/include/gconv.h \
 /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h \
 /usr/include/bits/stdio_lim.h /usr/include/errno.h \
 /usr/include/bits/errno.h /usr/include/linux/errno.h \
 /usr/include/asm/errno.h /usr/include/string.h /usr/include/xlocale.h \
 /usr/include/sys/types.h /usr/include/time.h /usr/include/endian.h \
 /usr/include/bits/endian.h /usr/include/sys/select.h \
 /usr/include/bits/select.h /usr/include/bits/sigset.h \
 /usr/include/bits/time.h /usr/include/sys/sysmacros.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/environments.h /usr/include/bits/wordsize.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h \
 /usr/include/sys/stat.h /usr/include/bits/stat.h \
 /usr/include/sys/mman.h /usr/include/bits/mman.h \
 /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h \
 /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h \
 /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h \
 ../dev/pci/sis1100_var.h /usr/include/linux/ioctl.h \
 ../sis3100_calls/sis3100_vme_calls.h header/sis9200.h header/imago.h

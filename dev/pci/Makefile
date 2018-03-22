# $ZEL: Makefile,v 1.10 2010/01/21 21:47:44 wuestner Exp $
#
# We're just setting the stage here and
# let the Kernel Makefile do the real work.
#

 KERNVER := $(shell uname -r)
 MODULENAME := sis1100

#16NOV2016 For FEDORA23_Kernel4.2.3.300
 ccflags-y  = -I/usr/src/kernels/$(KERNVER)/include/linux

#04APR2017 For UBUNTU16.04.1_Kernel4.8.0-36
 ccflags-y += -I/usr/src/linux-headers-$(KERNVER)/include/linux

#01NOV2017 For DEBIAN
 ccflags-y += -I/usr/src/linux-headers-$(shell uname -r | grep -Po '(.*)-')common/include/linux/

.PHONY: driver

driver:
	make -C /lib/modules/$(KERNVER)/build SUBDIRS=$(shell pwd) modules

install:
	-/sbin/rmmod $(MODULENAME)
	make -C /lib/modules/$(KERNVER)/build SUBDIRS=$(shell pwd) modules_install
	/sbin/depmod -a
	/sbin/modprobe $(MODULENAME)
	cp -f ../../udev-rules/92-struck.rules /etc/udev/rules.d/92-struck.rules

clean:
	-rm -f *.o *.ko .*.o.cmd .*.ko.cmd *.mod.c Module.symvers modules.order
	-rm -f Module.markers
	-rm -r -f .tmp_versions


ifndef src
    src:=$(shell pwd)
endif

SOURCES:=$(notdir $(wildcard $(src)/*.c))
sis1100-objs := $(patsubst %.c, %.o, $(filter-out %netbsd.c, $(SOURCES)))

ifneq ($(KERNELRELEASE), )

EXTRA_CFLAGS += -DMODULENAME=$(MODULENAME)

obj-m += sis1100.o

else

modules: sis1100.o

sis1100.o: $(sis1100-objs)
	$(LD) -r -o $@ $^

endif
